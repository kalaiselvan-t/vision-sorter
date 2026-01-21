from fastapi import FastAPI, HTTPException, BackgroundTasks
from typing import List
import os
import uuid
from . import models, database, storage, export_service

app = FastAPI(
    title="Intrinsic Data Pipeline API",
    description="REST API for accessing robotics data collected for the Intrinsic-Foxconn vision",
    version="1.0.0"
)

# Initialize clients
db_client = database.DatabaseClient()
storage_client = storage.StorageClient()
exporter = export_service.ExportService(db_client, storage_client)

@app.get("/")
async def root():
    return {
        "message": "Intrinsic Data Pipeline API is running",
        "vision": "Intelligent Factories of the Future",
        "docs": "/docs"
    }

@app.get("/episodes", response_model=List[models.Episode])
async def list_episodes(limit: int = 100, task_type: str = None):
    try:
        episodes = db_client.get_episodes(limit=limit, task_type=task_type)
        return episodes
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/episodes/{episode_id}", response_model=models.Episode)
async def get_episode(episode_id: int):
    episode = db_client.get_episode(episode_id)
    if not episode:
        raise HTTPException(status_code=404, detail="Episode not found")
    return episode

@app.get("/episodes/{episode_id}/video_url")
async def get_video_url(episode_id: int):
    episode = db_client.get_episode(episode_id)
    if not episode:
        raise HTTPException(status_code=404, detail="Episode not found")
    
    try:
        object_name = f"episodes/{episode['episode_name']}/rgb_camera.mp4"
        url = storage_client.get_presigned_url(episode['minio_bucket'], object_name)
        return {"url": url}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error generating presigned URL: {str(e)}")

@app.get("/episodes/{episode_id}/data", response_model=List[models.RobotState])
async def get_episode_data(episode_id: int):
    try:
        states = db_client.get_robot_states(episode_id)
        return states
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/datasets/export", status_code=202)
async def export_dataset(request: models.DataExportRequest, background_tasks: BackgroundTasks):
    # Generate a unique filename for the export
    export_id = str(uuid.uuid4())[:8]
    filename = f"dataset_{export_id}.h5"
    
    # Trigger background task
    background_tasks.add_task(
        exporter.generate_hdf5, 
        request.episode_ids, 
        filename
    )
    
    return {
        "status": "accepted",
        "export_id": export_id,
        "filename": filename,
        "message": f"HDF5 dataset generation started. Download from /datasets/{filename} when ready."
    }

@app.get("/datasets/{filename}/url")
async def get_dataset_url(filename: str):
    """Get a presigned URL to download a processed dataset"""
    try:
        url = storage_client.get_presigned_url("processed-datasets", f"exports/{filename}")
        return {"url": url}
    except Exception as e:
        raise HTTPException(status_code=404, detail="Dataset not found or still processing")

from fastapi import FastAPI, HTTPException, BackgroundTasks
from typing import List
import os
import uuid
from . import models, database, storage, lerobot_exporter

app = FastAPI(
    title="Intrinsic Data Pipeline API",
    description="REST API for accessing robotics data collected for the Intrinsic-Foxconn vision",
    version="1.0.0"
)

# Initialize clients
db_client = database.DatabaseClient()
storage_client = storage.StorageClient()
exporter = lerobot_exporter.LeRobotExportService(db_client, storage_client)

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
    # Generate a unique dataset name
    dataset_id = str(uuid.uuid4())[:8]
    dataset_name = f"lerobot_dataset_{dataset_id}"
    
    # Trigger background task for LeRobot format
    background_tasks.add_task(
        exporter.generate_lerobot_dataset, 
        request.episode_ids, 
        dataset_name
    )
    
    return {
        "status": "accepted",
        "dataset_name": dataset_name,
        "format": "lerobot_v3",
        "message": f"LeRobot v3.0 dataset generation started. Shards will be available in MinIO under lerobot/{dataset_name}/"
    }

@app.get("/datasets/{dataset_name}/status")
async def get_dataset_status(dataset_name: str):
    """Check if the dataset exists in MinIO"""
    try:
        # Check for meta/info.json as a proxy for completion
        self.storage.client.stat_object("processed-datasets", f"lerobot/{dataset_name}/meta/info.json")
        return {"status": "complete", "dataset_name": dataset_name}
    except Exception:
        return {"status": "processing_or_not_found"}

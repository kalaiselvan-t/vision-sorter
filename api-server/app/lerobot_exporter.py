import pandas as pd
import numpy as np
import os
import json
import tempfile
import shutil
from datetime import datetime
from .database import DatabaseClient
from .storage import StorageClient

class LeRobotExportService:
    def __init__(self, db_client: DatabaseClient, storage_client: StorageClient):
        self.db = db_client
        self.storage = storage_client

    def log(self, dataset_name, msg):
        with open("/app/app/export_debug.log", "a") as f:
            f.write(f"[{datetime.now()}] [{dataset_name}] {msg}\n")

    def generate_lerobot_dataset(self, episode_ids: list, dataset_name: str):
        """
        Gathers data for multiple episodes and packs them into LeRobot v3.0 format.
        """
        self.log(dataset_name, f"Starting export for episodes: {episode_ids}")
        # Create a temporary root directory for the dataset
        tmp_dir = tempfile.mkdtemp()
        data_dir = os.path.join(tmp_dir, "data/chunk-000")
        meta_dir = os.path.join(tmp_dir, "meta")
        videos_dir = os.path.join(tmp_dir, "videos")
        
        os.makedirs(data_dir, exist_ok=True)
        os.makedirs(meta_dir, exist_ok=True)
        os.makedirs(videos_dir, exist_ok=True)

        try:
            all_frames = []
            episodes_meta = []
            total_frames = 0
            camera_names = set()

            # 1. Process each episode
            for idx, ep_id in enumerate(episode_ids):
                self.log(dataset_name, f"Processing episode {ep_id}")
                ep_meta = self.db.get_episode(ep_id)
                if not ep_meta:
                    self.log(dataset_name, f"Episode {ep_id} not found in DB")
                    continue
                
                states = self.db.get_robot_states(ep_id)
                if not states:
                    self.log(dataset_name, f"No states for episode {ep_id}")
                    continue

                # Handle Videos
                streams = self.db.get_camera_streams(ep_id)
                for s in streams:
                    cam_name = s["camera_name"]
                    camera_names.add(cam_name)
                    cam_dir = os.path.join(videos_dir, f"observation.images.{cam_name}")
                    os.makedirs(cam_dir, exist_ok=True)
                    
                    # Download video from MinIO
                    # LeRobot v3.0 expects videos to be organized by camera
                    # We'll save as episode_{idx}.mp4 for simplicity per episode
                    video_filename = f"episode_{idx}.mp4"
                    local_video_path = os.path.join(cam_dir, video_filename)
                    
                    try:
                        self.storage.client.fget_object(
                            "raw-episodes", 
                            s["minio_object_path"], 
                            local_video_path
                        )
                        self.log(dataset_name, f"Downloaded video {s['minio_object_path']} to {local_video_path}")
                    except Exception as ve:
                        self.log(dataset_name, f"Failed to download video {s['minio_object_path']}: {str(ve)}")

                ep_start_frame = total_frames
                for frame_idx, s in enumerate(states):
                    frame_data = {
                        "index": total_frames,
                        "episode_index": idx,
                        "frame_index": frame_idx,
                        "timestamp": (s["time"] - states[0]["time"]).total_seconds(),
                        "observation.state": np.array([
                            s["joint_1"], s["joint_2"], s["joint_3"], 
                            s["joint_4"], s["joint_5"], s["joint_6"], s["joint_7"]
                        ], dtype=np.float32),
                        "action": np.array([
                            s["joint_1"], s["joint_2"], s["joint_3"], 
                            s["joint_4"], s["joint_5"], s["joint_6"], s["joint_7"]
                        ], dtype=np.float32)
                    }
                    all_frames.append(frame_data)
                    total_frames += 1

                episodes_meta.append({
                    "episode_index": idx,
                    "start_frame": ep_start_frame,
                    "end_frame": total_frames,
                    "task_type": ep_meta["task_type"],
                    "success": bool(ep_meta["success"])
                })

            if total_frames == 0:
                self.log(dataset_name, "No data collected across all episodes. Aborting.")
                return

            self.log(dataset_name, f"Total frames: {total_frames}")

            # 2. Write Data Parquet (Chunk 000)
            df = pd.DataFrame(all_frames)
            parquet_path = os.path.join(data_dir, "file-000.parquet")
            df.to_parquet(parquet_path, index=False)
            self.log(dataset_name, "Data parquet written")

            # 3. Write Episode Metadata Parquet
            ep_df = pd.DataFrame(episodes_meta)
            ep_parquet_dir = os.path.join(meta_dir, "episodes/chunk-000")
            os.makedirs(ep_parquet_dir, exist_ok=True)
            ep_df.to_parquet(os.path.join(ep_parquet_dir, "file-000.parquet"), index=False)
            self.log(dataset_name, "Episode metadata parquet written")

            # 4. Generate info.json
            features = {
                "observation.state": {"dtype": "float32", "shape": [7]},
                "action": {"dtype": "float32", "shape": [7]},
                "timestamp": {"dtype": "float32", "shape": []}
            }
            # Add image features
            for cam in camera_names:
                features[f"observation.images.{cam}"] = {
                    "dtype": "video",
                    "shape": [3, 480, 640], # Assuming default, ideal to get from DB
                    "names": ["channels", "height", "width"]
                }

            info = {
                "name": dataset_name,
                "version": "3.0",
                "fps": 30,
                "features": features,
                "total_frames": total_frames,
                "total_episodes": len(episodes_meta)
            }
            with open(os.path.join(meta_dir, "info.json"), "w") as f:
                json.dump(info, f, indent=2)

            # 5. Generate stats.json
            state_data = np.stack(df["observation.state"].values)
            action_data = np.stack(df["action"].values)
            
            stats = {
                "observation.state": {
                    "mean": state_data.mean(axis=0).tolist(),
                    "std": (state_data.std(axis=0) + 1e-6).tolist(), # Avoid div by zero
                    "min": state_data.min(axis=0).tolist(),
                    "max": state_data.max(axis=0).tolist(),
                },
                "action": {
                    "mean": action_data.mean(axis=0).tolist(),
                    "std": (action_data.std(axis=0) + 1e-6).tolist(),
                    "min": action_data.min(axis=0).tolist(),
                    "max": action_data.max(axis=0).tolist(),
                }
            }
            with open(os.path.join(meta_dir, "stats.json"), "w") as f:
                json.dump(stats, f, indent=2)

            # 6. Upload
            bucket = "processed-datasets"
            dataset_root = f"lerobot/{dataset_name}"
            
            # Ensure bucket exists
            try:
                if not self.storage.client.bucket_exists(bucket):
                    self.storage.client.make_bucket(bucket)
            except Exception as be:
                self.log(dataset_name, f"Bucket check failed: {str(be)}")

            for root, dirs, files in os.walk(tmp_dir):
                for file in files:
                    local_path = os.path.join(root, file)
                    rel_path = os.path.relpath(local_path, tmp_dir)
                    minio_path = f"{dataset_root}/{rel_path}"
                    self.storage.client.fput_object(bucket, minio_path, local_path)
            
            self.log(dataset_name, "Upload complete")

        except Exception as e:
            import traceback
            self.log(dataset_name, f"CRASH: {str(e)}\n{traceback.format_exc()}")
        finally:
            shutil.rmtree(tmp_dir)
            self.log(dataset_name, "Cleanup complete")

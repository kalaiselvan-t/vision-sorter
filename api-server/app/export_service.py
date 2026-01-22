import h5py
import numpy as np
import os
import tempfile
from datetime import datetime
from .database import DatabaseClient
from .storage import StorageClient

class ExportService:
    def __init__(self, db_client: DatabaseClient, storage_client: StorageClient):
        self.db = db_client
        self.storage = storage_client

    async def generate_hdf5(self, episode_ids: list, output_filename: str):
        """
        Gathers data for multiple episodes and packs them into a single HDF5 file.
        This runs in the background.
        """
        # Create a temporary file
        with tempfile.NamedTemporaryFile(suffix=".h5", delete=False) as tmp:
            tmp_path = tmp.name

        try:
            with h5py.File(tmp_path, "w") as f:
                # Add overall metadata
                f.attrs["creation_date"] = str(datetime.now())
                f.attrs["total_episodes"] = len(episode_ids)
                f.attrs["project"] = "Intrinsic-Foxconn Vision"

                episodes_group = f.create_group("episodes")

                for ep_id in episode_ids:
                    # 1. Fetch Episode Metadata
                    ep_meta = self.db.get_episode(ep_id)
                    if not ep_meta:
                        continue
                    
                    # Create group for this episode
                    ep_name = ep_meta["episode_name"]
                    ep_group = episodes_group.create_group(ep_name)
                    
                    # Store metadata as attributes
                    ep_group.attrs["task_type"] = ep_meta["task_type"]
                    ep_group.attrs["success"] = bool(ep_meta["success"])
                    ep_group.attrs["start_time"] = str(ep_meta["start_time"])
                    
                    # 2. Fetch Robot States
                    states = self.db.get_robot_states(ep_id)
                    if states:
                        # Convert to structured NumPy arrays
                        # Joints (timesteps, 7)
                        joints = np.array([
                            [s["joint_1"], s["joint_2"], s["joint_3"], s["joint_4"], 
                             s["joint_5"], s["joint_6"], s["joint_7"]] 
                            for s in states
                        ])
                        
                        # Timestamps (relative to start)
                        start_t = states[0]["time"]
                        timestamps = np.array([
                            (s["time"] - start_t).total_seconds() 
                            for s in states
                        ])

                        # Create datasets
                        ep_group.create_dataset("joint_positions", data=joints, compression="gzip")
                        ep_group.create_dataset("timestamps", data=timestamps, compression="gzip")

            # 3. Upload the final file to MinIO
            bucket = "processed-datasets"
            object_name = f"exports/{output_filename}"
            
            # Ensure bucket exists (simplified check)
            self.storage.download_file # (Using download_file as a proxy for client access)
            
            self.storage.client.fput_object(bucket, object_name, tmp_path)
            print(f"Dataset export complete: {object_name}")

        except Exception as e:
            print(f"Error during HDF5 export: {str(e)}")
        finally:
            # Clean up the local temp file
            if os.path.exists(tmp_path):
                os.remove(tmp_path)

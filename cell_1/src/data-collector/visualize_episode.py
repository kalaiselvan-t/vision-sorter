#!/usr/bin/env python3
"""
Visualize Episode Data
Downloads video from MinIO and generates plots from TimescaleDB for a specific episode.
"""

import os
import sys
import yaml
import psycopg2
import pandas as pd
import matplotlib.pyplot as plt
from minio import Minio
import argparse

def load_config():
    config_path = os.path.join(os.path.dirname(__file__), 'config.yaml')
    with open(config_path, 'r') as f:
        return yaml.safe_load(f)

def get_db_connection(config):
    db_cfg = config['storage']['timescaledb']
    return psycopg2.connect(
        host=db_cfg['host'],
        port=db_cfg['port'],
        database=db_cfg['database'],
        user=db_cfg['user'],
        password=db_cfg['password']
    )

def get_minio_client(config):
    minio_cfg = config['storage']['minio']
    return Minio(
        minio_cfg['endpoint'],
        access_key=minio_cfg['access_key'],
        secret_key=minio_cfg['secret_key'],
        secure=minio_cfg['secure']
    )

def list_episodes(conn):
    query = "SELECT id, episode_name, task_type, duration_seconds, start_time FROM episodes ORDER BY id DESC LIMIT 10"
    df = pd.read_sql(query, conn)
    print("\nRecent Episodes:")
    print(df.to_string(index=False))
    return df

def visualize_episode(episode_id, output_dir='.'):
    config = load_config()
    conn = get_db_connection(config)
    
    # 1. Get Episode Details
    cursor = conn.cursor()
    cursor.execute("SELECT episode_name, minio_bucket FROM episodes WHERE id = %s", (episode_id,))
    result = cursor.fetchone()
    if not result:
        print(f"Episode ID {episode_id} not found.")
        return
    episode_name, bucket = result
    print(f"\nVisualizing Episode: {episode_name} (ID: {episode_id})")
    
    # 2. Fetch Robot States
    print("Fetching robot states from TimescaleDB...")
    query = f"""
        SELECT time, joint_1, joint_2, joint_3, joint_4, joint_5, joint_6, joint_7 
        FROM robot_states 
        WHERE episode_id = {episode_id} 
        ORDER BY time ASC
    """
    df = pd.read_sql(query, conn)
    
    if df.empty:
        print("No robot state data found for this episode.")
    else:
        # Normalize time to start at 0
        df['relative_time'] = (df['time'] - df['time'].iloc[0]).dt.total_seconds()
        
        # Plot Joint Positions
        plt.figure(figsize=(12, 6))
        for i in range(1, 8):
            plt.plot(df['relative_time'].values, df[f'joint_{i}'].values, label=f'Joint {i}')
        
        plt.title(f'Joint Positions - {episode_name}')
        plt.xlabel('Time (s)')
        plt.ylabel('Position (rad)')
        plt.legend()
        plt.grid(True)
        
        plot_path = os.path.join(output_dir, f"{episode_name}_joints.png")
        plt.savefig(plot_path)
        print(f"Saved joint plot to: {plot_path}")
        plt.close()

    # 3. Download Video
    client = get_minio_client(config)
    video_path = f"episodes/{episode_name}/rgb_camera.mp4"
    local_video_path = os.path.join(output_dir, f"{episode_name}_video.mp4")
    
    print(f"Downloading video from MinIO: {video_path}...")
    try:
        client.fget_object(bucket, video_path, local_video_path)
        print(f"Saved video to: {local_video_path}")
    except Exception as e:
        print(f"Could not download video: {e}")
        print("Note: If you are running this inside a container without camera data, video might not exist.")

    conn.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Visualize Episode Data")
    parser.add_argument("episode_id", nargs="?", type=int, help="ID of the episode to visualize")
    parser.add_argument("--list", action="store_true", help="List recent episodes")
    
    args = parser.parse_args()
    
    try:
        conf = load_config()
        connection = get_db_connection(conf)
        
        if args.list or not args.episode_id:
            list_episodes(connection)
            if not args.episode_id:
                print("\nUsage: python3 visualize_episode.py <episode_id>")
        
        if args.episode_id:
            visualize_episode(args.episode_id, output_dir='/data_collection')
            
        connection.close()
    except Exception as e:
        print(f"Error: {e}")

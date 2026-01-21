#!/usr/bin/env python3
"""
Data Collector Node - Phase 1
Collects data from Franka ROS2 topics and stores in MinIO + TimescaleDB
"""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState, Image
from geometry_msgs.msg import PoseStamped, WrenchStamped
from cv_bridge import CvBridge
import cv2
import numpy as np
from datetime import datetime
import yaml
import os
import psycopg2
from minio import Minio
from minio.error import S3Error
import io
import json
from collections import deque
from threading import Lock, Thread
import time

class DataCollectorNode(Node):
    """Main data collection node for Phase 1"""
    
    def __init__(self):
        super().__init__('data_collector_node')
        
        # Load configuration
        config_path = os.path.join(os.path.dirname(__file__), 'config.yaml')
        with open(config_path, 'r') as f:
            self.config = yaml.safe_load(f)
        
        self.get_logger().info("=" * 60)
        self.get_logger().info("Data Collector Node - Phase 1 Starting")
        self.get_logger().info("=" * 60)
        
        # Initialize storage clients
        self._init_storage()
        
        # Episode management
        self.episode_id = None
        self.episode_name = None
        self.episode_start_time = None
        self.collecting = False
        
        # Data buffers (thread-safe)
        self.joint_states_buffer = deque(maxlen=1000)
        self.ee_poses_buffer = deque(maxlen=1000)
        self.force_torque_buffer = deque(maxlen=1000)
        self.buffer_lock = Lock()
        
        # Video recording
        self.cv_bridge = CvBridge()
        self.video_frames = []
        self.video_timestamps = []
        
        # Initialize ROS2 subscriptions
        self._init_subscribers()
        
        # Start background upload thread
        self.upload_thread = Thread(target=self._upload_worker, daemon=True)
        self.upload_thread.start()
        
        self.get_logger().info("Data Collector initialized successfully")
        self.get_logger().info("Ready to collect data. Use 'start_episode' service to begin.")
    
    def _init_storage(self):
        """Initialize MinIO and TimescaleDB connections"""
        # MinIO client
        minio_cfg = self.config['storage']['minio']
        self.minio_client = Minio(
            minio_cfg['endpoint'],
            access_key=minio_cfg['access_key'],
            secret_key=minio_cfg['secret_key'],
            secure=minio_cfg['secure']
        )
        
        # Check MinIO connection
        try:
            if not self.minio_client.bucket_exists(minio_cfg['bucket']):
                self.get_logger().error(f"Bucket '{minio_cfg['bucket']}' does not exist!")
            else:
                self.get_logger().info(f"Connected to MinIO bucket: {minio_cfg['bucket']}")
        except S3Error as e:
            self.get_logger().error(f"MinIO connection failed: {e}")
        
        # TimescaleDB connection
        db_cfg = self.config['storage']['timescaledb']
        try:
            self.db_conn = psycopg2.connect(
                host=db_cfg['host'],
                port=db_cfg['port'],
                database=db_cfg['database'],
                user=db_cfg['user'],
                password=db_cfg['password']
            )
            self.db_conn.autocommit = False
            self.get_logger().info(f"Connected to TimescaleDB: {db_cfg['database']}")
        except Exception as e:
            self.get_logger().error(f"TimescaleDB connection failed: {e}")
            self.db_conn = None
    
    def _init_subscribers(self):
        """Initialize ROS2 topic subscribers"""
        topics_cfg = self.config['topics']
        
        # Joint states subscriber
        self.create_subscription(
            JointState,
            topics_cfg['joint_states']['topic'],
            self._joint_states_callback,
            topics_cfg['joint_states']['queue_size']
        )
        self.get_logger().info(f"Subscribed to: {topics_cfg['joint_states']['topic']}")
        
        # End-effector pose subscriber
        self.create_subscription(
            PoseStamped,
            topics_cfg['ee_pose']['topic'],
            self._ee_pose_callback,
            topics_cfg['ee_pose']['queue_size']
        )
        self.get_logger().info(f"Subscribed to: {topics_cfg['ee_pose']['topic']}")
        
        # Force/torque subscriber
        self.create_subscription(
            WrenchStamped,
            topics_cfg['force_torque']['topic'],
            self._force_torque_callback,
            topics_cfg['force_torque']['queue_size']
        )
        self.get_logger().info(f"Subscribed to: {topics_cfg['force_torque']['topic']}")
        
        # RGB camera subscriber
        self.create_subscription(
            Image,
            topics_cfg['rgb_camera']['topic'],
            self._rgb_camera_callback,
            topics_cfg['rgb_camera']['queue_size']
        )
        self.get_logger().info(f"Subscribed to: {topics_cfg['rgb_camera']['topic']}")
    
    def start_episode(self, task_type='unknown'):
        """Start a new data collection episode"""
        if self.collecting:
            self.get_logger().warn("Already collecting data!")
            return False
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        self.episode_name = f"{self.config['collection']['episode_prefix']}_{timestamp}"
        self.episode_start_time = datetime.now()
        
        # Create episode in database
        if self.db_conn:
            try:
                cursor = self.db_conn.cursor()
                cursor.execute("""
                    INSERT INTO episodes (episode_name, task_type, start_time, minio_bucket, minio_prefix)
                    VALUES (%s, %s, %s, %s, %s)
                    RETURNING id
                """, (
                    self.episode_name,
                    task_type,
                    self.episode_start_time,
                    self.config['storage']['minio']['bucket'],
                    f"episodes/{self.episode_name}/"
                ))
                self.episode_id = cursor.fetchone()[0]
                self.db_conn.commit()
                cursor.close()
                
                self.get_logger().info("=" * 60)
                self.get_logger().info(f"Started Episode: {self.episode_name} (ID: {self.episode_id})")
                self.get_logger().info(f"  Task: {task_type}")
                self.get_logger().info("=" * 60)
            except Exception as e:
                self.get_logger().error(f"Failed to create episode in DB: {e}")
                return False
        
        self.collecting = True
        return True
    
    def stop_episode(self, success=None):
        """Stop current episode and finish uploading"""
        if not self.collecting:
            self.get_logger().warn("Not currently collecting data!")
            return False
        
        self.collecting = False
        episode_end_time = datetime.now()
        duration = (episode_end_time - self.episode_start_time).total_seconds()
        
        # Flush remaining buffers
        self._flush_buffers()
        
        # Upload video
        if self.video_frames:
            self._upload_video()
        
        # Update episode in database
        if self.db_conn and self.episode_id:
            try:
                cursor = self.db_conn.cursor()
                cursor.execute("""
                    UPDATE episodes
                    SET end_time = %s, duration_seconds = %s, success = %s, total_frames = %s
                    WHERE id = %s
                """, (episode_end_time, duration, success, len(self.video_timestamps), self.episode_id))
                self.db_conn.commit()
                cursor.close()
                
                self.get_logger().info("=" * 60)
                self.get_logger().info(f"Stopped Episode: {self.episode_name}")
                self.get_logger().info(f"  Duration: {duration:.2f}s")
                self.get_logger().info(f"  Total frames: {len(self.video_timestamps)}")
                self.get_logger().info("=" * 60)
            except Exception as e:
                self.get_logger().error(f"Failed to update episode in DB: {e}")
        
        # Reset buffers
        with self.buffer_lock:
            self.joint_states_buffer.clear()
            self.ee_poses_buffer.clear()
            self.force_torque_buffer.clear()
            self.video_frames.clear()
            self.video_timestamps.clear()
        
        return True
    
    def _joint_states_callback(self, msg):
        """Callback for joint states"""
        if not self.collecting:
            return
        
        timestamp = self.get_clock().now().to_msg()
        with self.buffer_lock:
            self.joint_states_buffer.append((timestamp, msg))
    
    def _ee_pose_callback(self, msg):
        """Callback for end-effector pose"""
        if not self.collecting:
            return
        
        with self.buffer_lock:
            self.ee_poses_buffer.append(msg)
    
    def _force_torque_callback(self, msg):
        """Callback for force/torque sensor"""
        if not self.collecting:
            return
        
        with self.buffer_lock:
            self.force_torque_buffer.append(msg)
    
    def _rgb_camera_callback(self, msg):
        """Callback for RGB camera"""
        if not self.collecting:
            return
        
        try:
            # Convert ROS Image to OpenCV
            cv_image = self.cv_bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
            timestamp = self.get_clock().now().to_msg()
            
            with self.buffer_lock:
                self.video_frames.append(cv_image)
                self.video_timestamps.append(timestamp)
        except Exception as e:
            self.get_logger().error(f"Image conversion failed: {e}")
    
    def _upload_worker(self):
        """Background thread to upload data to storage"""
        while rclpy.ok():
            time.sleep(1.0)  # Upload every second
            
            if self.collecting and self.episode_id:
                self._flush_buffers()
    
    def _flush_buffers(self):
        """Upload buffered data to TimescaleDB"""
        if not self.db_conn or not self.episode_id:
            return
        
        with self.buffer_lock:
            # Upload joint states
            if self.joint_states_buffer:
                try:
                    cursor = self.db_conn.cursor()
                    for timestamp, msg in list(self.joint_states_buffer):
                        # Convert ROS timestamp to Python datetime
                        ts = datetime.fromtimestamp(timestamp.sec + timestamp.nanosec / 1e9)
                        
                        # Assuming 7-DOF robot (Franka)
                        positions = list(msg.position[:7]) + [None] * (7 - len(msg.position[:7]))
                        velocities = list(msg.velocity[:7]) + [None] * (7 - len(msg.velocity[:7]))
                        efforts = list(msg.effort[:7]) + [None] * (7 - len(msg.effort[:7]))
                        
                        cursor.execute("""
                            INSERT INTO robot_states 
                            (time, episode_id, joint_1, joint_2, joint_3, joint_4, joint_5, joint_6, joint_7,
                             vel_1, vel_2, vel_3, vel_4, vel_5, vel_6, vel_7,
                             effort_1, effort_2, effort_3, effort_4, effort_5, effort_6, effort_7)
                            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                        """, (ts, self.episode_id, *positions, *velocities, *efforts))
                    
                    self.db_conn.commit()
                    cursor.close()
                    count = len(self.joint_states_buffer)
                    self.joint_states_buffer.clear()
                    self.get_logger().debug(f"Uploaded {count} joint states")
                except Exception as e:
                    self.get_logger().error(f"Failed to upload joint states: {e}")
                    self.db_conn.rollback()
            
            # Similar logic for ee_poses and force_torque (omitted for brevity in Phase 1)
    
    def _upload_video(self):
        """Upload collected video frames to MinIO"""
        if not self.video_frames or not self.episode_name:
            return
        
        self.get_logger().info(f"Encoding video with {len(self.video_frames)} frames...")
        
        try:
            # Define video writer
            height, width = self.video_frames[0].shape[:2]
            fourcc = cv2.VideoWriter_fourcc(*'mp4v')
            fps = self.config['collection']['video']['fps']
            
            output_path = f"/tmp/{self.episode_name}_rgb.mp4"
            writer = cv2.VideoWriter(output_path, fourcc, fps, (width, height))
            
            for frame in self.video_frames:
                writer.write(frame)
            
            writer.release()
            
            # Upload to MinIO
            object_name = f"episodes/{self.episode_name}/rgb_camera.mp4"
            self.minio_client.fput_object(
                self.config['storage']['minio']['bucket'],
                object_name,
                output_path
            )
            
            self.get_logger().info(f"Uploaded video: {object_name}")
            
            # Clean up local file
            os.remove(output_path)
        except Exception as e:
            self.get_logger().error(f"Video upload failed: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = DataCollectorNode()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down...")
    finally:
        if node.collecting:
            node.stop_episode()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()

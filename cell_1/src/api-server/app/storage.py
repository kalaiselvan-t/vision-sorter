import os
from minio import Minio
from datetime import timedelta

class StorageClient:
    def __init__(self):
        endpoint = os.getenv("MINIO_ENDPOINT", "localhost:9000")
        access_key = os.getenv("MINIO_ACCESS_KEY", "minioadmin")
        secret_key = os.getenv("MINIO_SECRET_KEY", "minioadmin123")
        secure = os.getenv("MINIO_SECURE", "False").lower() == "true"
        
        self.client = Minio(
            endpoint,
            access_key=access_key,
            secret_key=secret_key,
            secure=secure
        )

    def get_presigned_url(self, bucket: str, object_name: str):
        """Generate a presigned URL to download the video file"""
        return self.client.presigned_get_object(
            bucket, 
            object_name, 
            expires=timedelta(hours=1)
        )

    def download_file(self, bucket: str, object_name: str, file_path: str):
        self.client.fget_object(bucket, object_name, file_path)

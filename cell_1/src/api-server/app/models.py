from pydantic import BaseModel, Field
from typing import List, Optional
from datetime import datetime

class EpisodeBase(BaseModel):
    episode_name: str
    task_type: str
    start_time: datetime
    end_time: Optional[datetime] = None
    duration_seconds: Optional[float] = None
    success: Optional[bool] = None

class EpisodeCreate(EpisodeBase):
    pass

class Episode(EpisodeBase):
    id: int
    minio_bucket: Optional[str] = None
    total_frames: int = 0
    total_size_bytes: int = 0

    class Config:
        from_attributes = True

class RobotState(BaseModel):
    time: datetime
    joint_1: float
    joint_2: float
    joint_3: float
    joint_4: float
    joint_5: float
    joint_6: float
    joint_7: float

class DataExportRequest(BaseModel):
    episode_ids: List[int]
    format: str = Field(default="hdf5", pattern="^(hdf5|tfrecord|webdataset)$")

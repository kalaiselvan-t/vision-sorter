import os
import psycopg2
from psycopg2.extras import RealDictCursor
from . import models

class DatabaseClient:
    def __init__(self):
        self.host = os.getenv("DB_HOST", "localhost")
        self.port = os.getenv("DB_PORT", "5432")
        self.database = os.getenv("DB_NAME", "robotdata")
        self.user = os.getenv("DB_USER", "datauser")
        self.password = os.getenv("DB_PASS", "datapass123")

    def get_connection(self):
        return psycopg2.connect(
            host=self.host,
            port=self.port,
            database=self.database,
            user=self.user,
            password=self.password,
            cursor_factory=RealDictCursor
        )

    def get_episodes(self, limit: int = 100, task_type: str = None, success: bool = None):
        with self.get_connection() as conn:
            with conn.cursor() as cur:
                query = "SELECT * FROM episodes"
                params = []
                conditions = []
                
                if task_type:
                    conditions.append("task_type = %s")
                    params.append(task_type)
                
                if success is not None:
                    if success is False:
                        conditions.append("(success = %s OR success IS NULL)")
                    else:
                        conditions.append("success = %s")
                    params.append(success)
                
                if conditions:
                    query += " WHERE " + " AND ".join(conditions)
                
                query += " ORDER BY start_time DESC LIMIT %s"
                params.append(limit)
                
                cur.execute(query, params)
                return cur.fetchall()

    def get_episode(self, episode_id: int):
        with self.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute("SELECT * FROM episodes WHERE id = %s", (episode_id,))
                return cur.fetchone()

    def get_robot_states(self, episode_id: int):
        with self.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute(
                    "SELECT * FROM robot_states WHERE episode_id = %s ORDER BY time ASC",
                    (episode_id,)
                )
                return cur.fetchall()

    def get_camera_streams(self, episode_id: int):
        with self.get_connection() as conn:
            with conn.cursor() as cur:
                cur.execute(
                    "SELECT * FROM camera_streams WHERE episode_id = %s",
                    (episode_id,)
                )
                return cur.fetchall()

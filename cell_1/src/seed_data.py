import psycopg2
import uuid
import random
import time

# Database connection parameters
DB_HOST = "localhost"
DB_PORT = "5432"
DB_NAME = "robotdata"
DB_USER = "datauser"
DB_PASS = "datapass123"

def seed_data():
    try:
        conn = psycopg2.connect(
            host=DB_HOST,
            port=DB_PORT,
            dbname=DB_NAME,
            user=DB_USER,
            password=DB_PASS
        )
        cur = conn.cursor()
        
        print("Connected to database...")

        # Generate some mock episodes
        tasks = ["pick_place_cube", "sort_cylinder", "stack_blocks"]
        
        for i in range(20):
            episode_id = i + 1000
            name = f"episode_{uuid.uuid4().hex[:8]}"
            timestamp = time.time() - (i * 3600) # Spread out over time
            success = random.choice([True, True, True, False]) # 75% success
            duration = random.uniform(15.0, 45.0)
            task = random.choice(tasks)
            
            # Insert episode
            cur.execute("""
                INSERT INTO episodes (episode_name, start_time, success, duration_seconds, task_type, minio_bucket)
                VALUES (%s, to_timestamp(%s), %s, %s, %s, %s)
                ON CONFLICT DO NOTHING
            """, (name, timestamp, success, duration, task, "raw-episodes"))
            
        conn.commit()
        print("Successfully seeded 20 mock episodes.")
        
        cur.close()
        conn.close()
        
    except Exception as e:
        print(f"Error seeding data: {e}")

if __name__ == "__main__":
    seed_data()

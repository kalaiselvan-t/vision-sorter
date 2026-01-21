# Data Collection Pipeline - Phase 1

## 🎯 Phase 1 Scope

**Goal**: Core data collection, synchronization, and storage infrastructure

**Components**:
- ✅ MinIO (S3-compatible object storage) for videos and large files
- ✅ TimescaleDB (PostgreSQL + time-series) for robot states and metadata
- ✅ ROS2 data collector node with multi-topic subscription
- ✅ Episode-based data organization

---

## 📋 Prerequisites

1. **Existing `franka_ros2` container** running with ROS2 topics
2. **Docker** and **Docker Compose** installed

---

## 🚀 Quick Start

### 1. Start Storage Infrastructure

```bash
cd /home/kalai/Documents/vision-sorter

# Start MinIO and TimescaleDB
docker-compose -f docker-compose.storage.yml up -d

# Verify containers are running
docker-compose -f docker-compose.storage.yml ps

# Expected output:
# - data-storage-minio (healthy)
# - data-storage-timescaledb (healthy)
# - minio-init (exited - this is normal, it's a one-time setup)
```

**Access Web Interfaces**:
- MinIO Console: http://localhost:9001 (user: `minioadmin`, pass: `minioadmin123`)
- TimescaleDB: `psql -h localhost -U datauser -d robotdata` (pass: `datapass123`)

---

### 2. Start Data Collector

The data collector runs in your existing `data_collection` container (which uses `network_mode: host`).

```bash
# Enter the data_collection container
docker exec -it data_collection /bin/bash

# Inside container:
cd /ros2_ws/src/data-collector

# Install Python dependencies (if not already in Dockerfile)
pip3 install minio psycopg2-binary opencv-python pyyaml cv-bridge

# Make collector executable
chmod +x collector_node.py test_collector.py

# Source ROS2
source /opt/ros/humble/setup.bash

# Start the collector node (runs in background, listens to topics)
python3 collector_node.py
```

---

### 3. Collect Data

In a **separate terminal**, trigger episode collection:

```bash
docker exec -it data_collection /bin/bash

cd /ros2_ws/src/data-collector
source /opt/ros/humble/setup.bash

# Start an episode
python3 test_collector.py start pick_place

# ... let your robot run ...

# Stop the episode
python3 test_collector.py stop true
```

---

## 🔍 Verify Data Collection

### Check  MinIO (Video Storage)

```bash
# Option 1: Web UI
# Visit http://localhost:9001
# Navigate to bucket 'raw-episodes' → 'episodes/' folder

# Option 2: CLI
docker exec minio-init mc ls myminio/raw-episodes/episodes/
```

### Check TimescaleDB (Robot States)

```bash
docker exec -it data-storage-timescaledb psql -U datauser -d robotdata

# Inside psql:
\dt                                    # List tables
SELECT * FROM episodes ORDER BY id DESC LIMIT 5;
SELECT COUNT(*) FROM robot_states;
SELECT time, joint_1, joint_2 FROM robot_states ORDER BY time DESC LIMIT 10;
\q
```

---

## 📊 ROS2 Topics

The collector subscribes to these topics (configured in `data-collector/config.yaml`):

| Topic | Message Type | Purpose |
|-------|--------------|---------|
| `/joint_states` | `sensor_msgs/JointState` | Robot joint positions, velocities, efforts |
| `/ee_pose` | `geometry_msgs/PoseStamped` | End-effector Cartesian pose |
| `/force_torque` | `geometry_msgs/WrenchStamped` | Contact forces/torques |
| `/camera/rgb/image_raw` | `sensor_msgs/Image` | RGB camera stream |

**Note**: Adjust topic names in `config.yaml` to match your `franka_ros2` setup.

---

## 🐛 Troubleshooting

### Data collector can't connect to storage

**Symptom**: Errors like "Connection refused" when starting collector

**Solution**: 
1. Verify storage containers are running: `docker-compose -f docker-compose.storage.yml ps`
2. Check ports are exposed: `docker ps | grep -E "minio|timescale"`
3. Confirm `data_collection` container is using `network_mode: host`

### No data in database

**Symptom**: Tables are empty after collecting

**Solution**:
1. Check if ROS2 topics are publishing: `ros2 topic list` and `ros2 topic echo /joint_states`
2. Verify `ROS_DOMAIN_ID=0` matches between `franka_ros2` and `data_collection`
3. Check collector logs for errors

### Video not uploading to MinIO

**Symptom**: No video files in MinIO after episode

**Solution**:
1. Verify camera topic is publishing images: `ros2 topic hz /camera/rgb/image_raw`
2. Check collector logs: might be encoding errors
3. Ensure `cv-bridge` is installed in `data_collection` container

---

## 📁 Project Structure

```
vision-sorter/
├── docker-compose.storage.yml       # Storage infrastructure
├── docker-compose.yml               # Existing franka_ros2 setup
├── data-collector/
│   ├── Dockerfile                   # (Optional) Collector container
│   ├── config.yaml                  # Configuration
│   ├── collector_node.py            # Main ROS2 node
│   └── test_collector.py            # Test script
└── storage/
    └── init-schema.sql              # TimescaleDB schema
```

---

## 🎯 Next Steps (Future Phases)

Phase 1 focuses on **core data collection**. Future phases will add:

- **Phase 2**: REST API for dataset querying and export
- **Phase 3**: Web dashboard for monitoring
- **Phase 4**: Dataset export to ML formats (HDF5, TFRecord)
- **Phase 5**: Multi-workcell scaling demonstration

---

## 📝 Configuration

Edit `data-collector/config.yaml` to customize:
- ROS2 topic names
- Storage endpoints
- Video encoding settings
- Buffer sizes
- Synchronization tolerance

---

## 🔧 Development

### Modify Topics

Edit `config.yaml` and restart the collector node.

### Database Schema

Schema is defined in `storage/init-schema.sql`. To modify:
1. Edit the SQL file
2. Drop and recreate the database container:
   ```bash
   docker-compose -f docker-compose.storage.yml down -v
   docker-compose -f docker-compose.storage.yml up -d
   ```

### Add New Data Streams

1. Add topic configuration to `config.yaml`
2. Add subscriber in `collector_node.py` → `_init_subscribers()`
3. Add callback to buffer the data
4. Add upload logic in `_flush_buffers()`

---

## 📞 Support

For questions about this setup, check:
- `implementation_plan.md` - Full architectural details
- `data-collection-scope.md` - Project scope and context

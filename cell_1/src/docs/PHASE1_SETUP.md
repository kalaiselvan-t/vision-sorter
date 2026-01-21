# Phase 1 Setup - Corrected Approach

## ✅ What You Were Right About

You correctly identified that **data-collector/Dockerfile is unnecessary** because:

1. **Message definitions**: The collector needs access to `franka_msgs` and any custom messages from your workspace
2. **Shared workspace**: Your existing `data_collection` container already shares `/ros2_ws/src` with `franka_ros2`
3. **Proper build context**: Both containers use the same build context, ensuring consistent ROS2 environment

## 📦 What We Actually Use

### Existing Infrastructure (Already Configured)
- ✅ `Dockerfile.datacollection` - Updated with Python dependencies
- ✅ `docker-compose.yml` - `data_collection` container with `network_mode: host`
- ✅ Shared workspace via volume mount: `./:/ros2_ws/src`

### New Components (Phase 1)
- ✅ `docker-compose.storage.yml` - MinIO + TimescaleDB
- ✅ `data-collector/collector_node.py` - ROS2 data collection node  
- ✅ `data-collector/config.yaml` - Configuration
- ✅ `storage/init-schema.sql` - Database schema

## 🚀 Deployment Steps

### 1. Rebuild data_collection Container

Since we updated `Dockerfile.datacollection` with new Python dependencies:

```bash
cd /home/kalai/Documents/vision-sorter

# Rebuild the container
docker-compose build data_collection

# Or rebuild and restart
docker-compose up -d --build data_collection
```

### 2. Start Storage Infrastructure

```bash
# Start MinIO and TimescaleDB
docker-compose -f docker-compose.storage.yml up -d

# Verify
docker-compose -f docker-compose.storage.yml ps
```

Expected output:
- ✅ `data-storage-minio` - healthy
- ✅ `data-storage-timescaledb` - healthy  
- ✅ `minio-init` - exited (normal - one-time setup)

### 3. Run Data Collector

```bash
# Enter data_collection container
docker exec -it data_collection /bin/bash

# Navigate to collector
cd /ros2_ws/src/data-collector

# Source ROS2 (should have franka_msgs if franka_ros2 is built)
source /opt/ros/humble/setup.bash

# If you've built your workspace:
source /ros2_ws/install/setup.bash

# Start collector node
python3 collector_node.py
```

**In another terminal, trigger collection:**

```bash
docker exec -it data_collection /bin/bash
cd /ros2_ws/src/data-collector
source /opt/ros/humble/setup.bash
source /ros2_ws/install/setup.bash  # if workspace is built

# Start episode
python3 test_collector.py start pick_place

# ... run your robot task ...

# Stop episode
python3 test_collector.py stop true
```

## 🔍 Why This Works

```
┌─────────────────────────────────────────────────┐
│ data_collection container                       │
│                                                  │
│  /ros2_ws/src/                                  │
│  ├── franka_ros2/          ← franka_msgs here!  │
│  ├── franka_description/                        │
│  ├── franka_msgs/                               │
│  ├── data-collector/       ← collector here!    │
│  │   ├── collector_node.py                      │
│  │   ├── test_collector.py                      │
│  │   └── config.yaml                            │
│  └── ... (other packages)                       │
│                                                  │
│  network_mode: "host"      ← Access to:         │
│  - ROS2 topics from franka_ros2                 │
│  - MinIO at localhost:9000                      │
│  - TimescaleDB at localhost:5432                │
└─────────────────────────────────────────────────┘
```

## 📋 Next Steps

1. **Verify ROS2 topics** are publishing from franka_ros2:
   ```bash
   docker exec -it data_collection bash
   source /opt/ros/humble/setup.bash
   ros2 topic list
   ros2 topic echo /joint_states --once
   ```

2. **Test storage connectivity**:
   ```bash
   # MinIO
   curl http://localhost:9000/minio/health/live
   
   # TimescaleDB
   docker exec -it data-storage-timescaledb psql -U datauser -d robotdata -c "\dt"
   ```

3. **Collect first episode** following steps above

4. **Verify data**:
   - MinIO: http://localhost:9001 (user: `minioadmin`, pass: `minioadmin123`)
   - Database: `SELECT * FROM episodes;`

## 🗑️ What to Ignore/Delete

- ❌ `data-collector/Dockerfile` - Not needed, use existing `Dockerfile.datacollection`

## 💡 Key Takeaway

Your existing Docker infrastructure is perfect for this! We just:
1. Added Python dependencies to `Dockerfile.datacollection`
2. Created the collector code in the shared workspace
3. Added storage infrastructure alongside it

The collector automatically has access to all ROS2 messages because it's in the same workspace as `franka_ros2`. 🎯

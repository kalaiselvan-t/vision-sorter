-- TimescaleDB Schema for Robot Data Collection
-- Phase 1: Core tables for episodes, time-series metrics, and metadata

-- Enable TimescaleDB extension
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- ============================================
-- Episodes Table: Metadata for each data collection episode
-- ============================================
CREATE TABLE IF NOT EXISTS episodes (
    id SERIAL PRIMARY KEY,
    episode_name VARCHAR(255) UNIQUE NOT NULL,
    task_type VARCHAR(100) NOT NULL,           -- e.g., 'pick_place', 'assembly', 'inspection'
    start_time TIMESTAMPTZ NOT NULL,
    end_time TIMESTAMPTZ,
    duration_seconds FLOAT,
    success BOOLEAN,
    
    -- Storage references
    minio_bucket VARCHAR(100),
    minio_prefix VARCHAR(500),                 -- Path prefix in MinIO (e.g., 'episodes/2024-01-20/ep001/')
    
    -- Data quality metrics
    total_frames INTEGER DEFAULT 0,
    missing_frames INTEGER DEFAULT 0,
    total_size_bytes BIGINT DEFAULT 0,
    
    -- Metadata
    notes TEXT,
    tags TEXT[],                               -- Array of tags for filtering
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_episodes_start_time ON episodes(start_time DESC);
CREATE INDEX idx_episodes_task_type ON episodes(task_type);
CREATE INDEX idx_episodes_success ON episodes(success);
CREATE INDEX idx_episodes_tags ON episodes USING GIN(tags);

-- ============================================
-- Robot States (Time-Series): High-frequency robot joint states
-- ============================================
CREATE TABLE IF NOT EXISTS robot_states (
    time TIMESTAMPTZ NOT NULL,
    episode_id INTEGER REFERENCES episodes(id) ON DELETE CASCADE,
    
    -- Joint positions (7-DOF Franka)
    joint_1 DOUBLE PRECISION,
    joint_2 DOUBLE PRECISION,
    joint_3 DOUBLE PRECISION,
    joint_4 DOUBLE PRECISION,
    joint_5 DOUBLE PRECISION,
    joint_6 DOUBLE PRECISION,
    joint_7 DOUBLE PRECISION,
    
    -- Joint velocities
    vel_1 DOUBLE PRECISION,
    vel_2 DOUBLE PRECISION,
    vel_3 DOUBLE PRECISION,
    vel_4 DOUBLE PRECISION,
    vel_5 DOUBLE PRECISION,
    vel_6 DOUBLE PRECISION,
    vel_7 DOUBLE PRECISION,
    
    -- Joint torques/efforts
    effort_1 DOUBLE PRECISION,
    effort_2 DOUBLE PRECISION,
    effort_3 DOUBLE PRECISION,
    effort_4 DOUBLE PRECISION,
    effort_5 DOUBLE PRECISION,
    effort_6 DOUBLE PRECISION,
    effort_7 DOUBLE PRECISION
);

-- Convert to hypertable (TimescaleDB magic for time-series optimization)
SELECT create_hypertable('robot_states', 'time', if_not_exists => TRUE);

-- Create index on episode_id for faster filtering
CREATE INDEX idx_robot_states_episode ON robot_states(episode_id, time DESC);

-- ============================================
-- End-Effector Poses (Time-Series): Cartesian pose of robot end-effector
-- ============================================
CREATE TABLE IF NOT EXISTS ee_poses (
    time TIMESTAMPTZ NOT NULL,
    episode_id INTEGER REFERENCES episodes(id) ON DELETE CASCADE,
    
    -- Position (meters)
    pos_x DOUBLE PRECISION,
    pos_y DOUBLE PRECISION,
    pos_z DOUBLE PRECISION,
    
    -- Orientation (quaternion)
    quat_x DOUBLE PRECISION,
    quat_y DOUBLE PRECISION,
    quat_z DOUBLE PRECISION,
    quat_w DOUBLE PRECISION
);

SELECT create_hypertable('ee_poses', 'time', if_not_exists => TRUE);
CREATE INDEX idx_ee_poses_episode ON ee_poses(episode_id, time DESC);

-- ============================================
-- Force/Torque Sensors (Time-Series): Contact forces
-- ============================================
CREATE TABLE IF NOT EXISTS force_torque (
    time TIMESTAMPTZ NOT NULL,
    episode_id INTEGER REFERENCES episodes(id) ON DELETE CASCADE,
    
    -- Force (Newtons)
    force_x DOUBLE PRECISION,
    force_y DOUBLE PRECISION,
    force_z DOUBLE PRECISION,
    
    -- Torque (Newton-meters)
    torque_x DOUBLE PRECISION,
    torque_y DOUBLE PRECISION,
    torque_z DOUBLE PRECISION
);

SELECT create_hypertable('force_torque', 'time', if_not_exists => TRUE);
CREATE INDEX idx_force_torque_episode ON force_torque(episode_id, time DESC);

-- ============================================
-- Camera Metadata: References to video streams stored in MinIO
-- ============================================
CREATE TABLE IF NOT EXISTS camera_streams (
    id SERIAL PRIMARY KEY,
    episode_id INTEGER REFERENCES episodes(id) ON DELETE CASCADE,
    camera_name VARCHAR(100) NOT NULL,         -- e.g., 'rgb_camera', 'depth_camera', 'wrist_camera'
    
    -- Video file reference in MinIO
    minio_object_path VARCHAR(500) NOT NULL,   -- e.g., 'episodes/2024-01-20/ep001/rgb_camera.mp4'
    
    -- Video metadata
    width INTEGER,
    height INTEGER,
    fps DOUBLE PRECISION,
    codec VARCHAR(50),                         -- e.g., 'H264', 'H265'
    duration_seconds DOUBLE PRECISION,
    file_size_bytes BIGINT,
    
    -- Timing info
    start_time TIMESTAMPTZ NOT NULL,
    end_time TIMESTAMPTZ,
    
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_camera_streams_episode ON camera_streams(episode_id);

-- ============================================
-- System Metrics (Time-Series): Performance monitoring
-- ============================================
CREATE TABLE IF NOT EXISTS system_metrics (
    time TIMESTAMPTZ NOT NULL,
    
    -- Resource usage
    cpu_percent DOUBLE PRECISION,
    memory_percent DOUBLE PRECISION,
    memory_mb DOUBLE PRECISION,
    disk_read_mbps DOUBLE PRECISION,
    disk_write_mbps DOUBLE PRECISION,
    network_sent_mbps DOUBLE PRECISION,
    network_recv_mbps DOUBLE PRECISION,
    
    -- Container/host identifier
    host_name VARCHAR(100)
);

SELECT create_hypertable('system_metrics', 'time', if_not_exists => TRUE);

-- ============================================
-- Data Quality Log: Track issues during collection
-- ============================================
CREATE TABLE IF NOT EXISTS quality_log (
    id SERIAL PRIMARY KEY,
    time TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    episode_id INTEGER REFERENCES episodes(id) ON DELETE CASCADE,
    
    severity VARCHAR(20),                      -- 'info', 'warning', 'error', 'critical'
    component VARCHAR(100),                    -- e.g., 'camera_sync', 'storage_upload', 'joint_states'
    message TEXT NOT NULL,
    
    metadata JSONB                             -- Additional context as JSON
);

CREATE INDEX idx_quality_log_episode ON quality_log(episode_id, time DESC);
CREATE INDEX idx_quality_log_severity ON quality_log(severity, time DESC);

-- ============================================
-- Retention Policies (Optional): Auto-delete old data
-- ============================================
-- Uncomment to enable automatic data retention (delete data older than 90 days)
-- SELECT add_retention_policy('robot_states', INTERVAL '90 days');
-- SELECT add_retention_policy('ee_poses', INTERVAL '90 days');
-- SELECT add_retention_policy('force_torque', INTERVAL '90 days');
-- SELECT add_retention_policy('system_metrics', INTERVAL '30 days');

-- ============================================
-- Continuous Aggregates (Optional): Pre-compute common queries
-- ============================================
-- Example: 1-second average of robot states for faster dashboard queries
-- CREATE MATERIALIZED VIEW robot_states_1sec
-- WITH (timescaledb.continuous) AS
-- SELECT time_bucket('1 second', time) AS bucket,
--        episode_id,
--        AVG(joint_1) as avg_joint_1,
--        AVG(joint_2) as avg_joint_2,
--        -- ... other joints
-- FROM robot_states
-- GROUP BY bucket, episode_id;

-- ============================================
-- Initial test episode (for validation)
-- ============================================
INSERT INTO episodes (episode_name, task_type, start_time, success, notes) 
VALUES ('test_episode_001', 'test', NOW(), TRUE, 'Initial test episode for schema validation')
ON CONFLICT (episode_name) DO NOTHING;

-- Success message
DO $$
BEGIN
    RAISE NOTICE 'TimescaleDB schema initialized successfully!';
    RAISE NOTICE 'Tables created: episodes, robot_states, ee_poses, force_torque, camera_streams, system_metrics, quality_log';
END $$;

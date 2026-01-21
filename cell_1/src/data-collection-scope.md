# Data Collection Pipeline - Intrinsic Interview Project

## Interview Context

**Company**: Intrinsic (Alphabet's robotics AI company)  
**Role**: Robotics Software Engineer - Large Scale Training  
**Project Goal**: Build a production-grade data collection pipeline demonstration that showcases skills relevant to the role

---

## About Intrinsic

### Core Mission
Intrinsic aims to make industrial robotics more intelligent, accessible, and usable for a wider range of businesses through AI and software platforms.

### Key Products & Platforms

#### **Intrinsic Flowstate**
- Web-based developer environment for building production-grade robotics solutions
- Digital twin simulation for testing before physical deployment
- Supports Python, C++, and low-code behavior trees
- AI-powered perception, motion planning, and sensor-based control

#### **Intrinsic Vision Model (IVM)**
- State-of-the-art industrial foundation model (announced Oct 2025)
- Pre-trained on 130,000+ items for object detection and 6D pose estimation
- Zero-shot learning capabilities
- Adapts to varying environmental conditions

#### **IntrinsicOS**
- Underlying operating system for seamless deployment from development to production

### ML/AI Capabilities
1. **Perception**: AI-powered pose estimators, object detection
2. **Motion Planning**: AI-driven collision-free path planning (~25% improvement over traditional methods)
3. **Sensor-based Control**: Real-time force/torque/distance sensor processing
4. **Foundation Models**: Integration with NVIDIA Isaac and Google DeepMind for manipulation
5. **Reinforcement Learning**: Multi-robot task planning with graph neural networks

### **STRATEGIC ANCHOR: Intrinsic-Foxconn Joint Venture** 🎯

**Announced**: November 2025 at Hon Hai Tech Day 2025

**Vision**: Build the "Intelligent Factory of the Future"

**Scale**: Multi-phase deployment across **230+ Foxconn global campuses** (starting in US)

**Core Problem Being Solved**:
- Traditional automation is **rigid, hard-coded, expensive** to adapt
- High-mix manufacturing (variable designs, small batches) has been **economically unviable** to automate
- Electronics assembly tasks are too **complex** for traditional robots
- Need for **general-purpose intelligent robotics** that can:
  - Adapt to variability without re-engineering
  - Learn from new data
  - Scale across production lines seamlessly

**Target Use Cases** (Initial Focus):
1. **Assembly**: Complex electronics assembly previously done by humans
2. **Inspection**: Adaptive quality control for high-variance production
3. **Machine Tending**: Flexible CNC and manufacturing equipment loading
4. **Logistics**: Warehouse and material handling automation

**Technology Integration**:
- Intrinsic's AI platform (Flowstate, IVM, IntrinsicOS)
- Foxconn's Smart Manufacturing platform
- NVIDIA partnerships: Digital twins (Omniverse), robotics (Isaac), humanoid robots
- Goal: Full factory orchestration and automation

**Business Impact**:
- Foxconn achieved **80%+ revenue per employee increase** through smart manufacturing
- Targeting **high-mix, high-volume production** (previously impossible to automate economically)
- Vision: From single workcells → full factories → 230 campuses worldwide

**Why This Matters for the Role**:
The "Large Scale Training" position exists to build the data infrastructure that will:
- Train foundation models for 230+ factories
- Collect diverse manipulation data across varied electronics assembly tasks
- Enable continuous learning from global manufacturing operations
- Scale model training across thousands of robot workcells

> [!IMPORTANT]
> **AI for Industry Challenge 2026** is a **tactical milestone** within this strategic vision:
> - Registration: Oct 27, 2025 - Apr 17, 2026
> - Challenge runs: Feb 11, 2026 (6 months)  
> - Focus: Dexterous cable management/insertion in electronics assembly
> - Prize pool: $180,000
> - **Purpose**: Crowdsource solutions for one of many difficult automation tasks needed for the factory vision

---

## Role: Robotics Software Engineer - Large Scale Training

### Key Responsibilities
1. **Infrastructure & Scaling**
   - Develop infrastructure to scale data collection across numerous robot workcells
   - Build tools for model training for Physical AI across industrial assembly tasks

2. **Data Collection Workflows**
   - Design data collection workflows
   - Orchestrate data acquisition from robot workcells in various timezones
   - Leverage cloud infrastructure for storage

3. **ML Pipeline**
   - Ensure dataset availability for ML training
   - Contribute to core training pipelines
   - Validate ML model performance across diverse tasks
   - Create high-quality, training-ready datasets

4. **Tooling**
   - Build internal tools (web frontends, CLIs) for data collection, inspection, and processing
   - Support and optimize ML training pipelines for scalability and reliability

5. **Hardware Integration**
   - Manage robot hardware experiments in manufacturing environments
   - Work in lab environments with mechatronic industrial hardware

### Required Skills
- Python or Go for software development
- Experience with physical experiments on industrial hardware
- Cross-functional collaboration in fast-paced environments
- Full-stack web engineering (TypeScript/JavaScript, C++, Go)
- Cloud microservice architectures (Kubernetes, Docker)
- API design and streaming architectures

---

## Project Architecture

**Goal**: Build a production data collection pipeline where a separate container collects data while the `franka_ros2` workcell container is operating.

**Container System**:
- Container 1: `franka_ros2` - Robotic workcell (simulating Intrinsic's industrial setup)
- Container 2: Data collection service (demonstrating large-scale training infrastructure)

---

## Tailored Project Scope (Aligned with Intrinsic's Needs)

### 1. Data Collection Scope ✅

**Target Data Types** (mimicking Intrinsic's Physical AI training needs):
- ✅ **Vision Data**: RGB-D camera streams for perception training (similar to IVM training)
- ✅ **Robot State**: Joint positions, velocities, torques, end-effector poses
- ✅ **Force/Torque Sensors**: Contact-rich manipulation data
- ✅ **ROS2 Topics**: All relevant robot topics for behavior cloning
- ✅ **System Metrics**: Performance monitoring across distributed workcells
- ✅ **Episode Metadata**: Task success/failure labels, annotations

**Rationale**: This mirrors the data needed for training foundation models and reinforcement learning policies like those used in Intrinsic's platform.

---

### 2. Data Volume & Rate ✅

**Expected Specifications**:
- **Cameras**: 2-3 cameras @ 640x480 resolution, 30 FPS (~50-75 MB/s)
- **Robot State**: 100 Hz (joint states, poses) (~1 MB/s)
- **Force/Torque**: 1 kHz sampling (~0.5 MB/s)
- **Total Data Rate**: ~50-80 MB/s during active collection
- **Storage**: 300-500 GB per 8-hour collection session

**Rationale**: Aligned with typical robotics learning datasets and realistic workcell operation.

---

### 3. Use Case & Primary Requirements ✅

**Purpose**: Training Physical AI models (Imitation Learning + RL)

**Specific Use Cases**:
1. **Imitation Learning**: Collect demonstration data for behavior cloning
2. **Foundation Model Training**: Build datasets for vision-based manipulation
3. **Multi-task Learning**: Collect diverse manipulation episodes
4. **Model Validation**: Gather test data for performance benchmarking

**Rationale**: Directly aligns with "Large Scale Training" role responsibilities at Intrinsic.

---

### 4. Processing Requirements ✅

**Real-time Processing**:
- ✅ Camera frame compression (H.264/H.265 encoding on-the-fly)
- ✅ Data synchronization across sensors (timestamp alignment)
- ✅ Episode segmentation (start/stop detection)
- ✅ Basic quality checks (missing data detection)

**Post-processing Pipeline**:
- Data validation and integrity checks
- Dataset statistics generation
- Training-ready format conversion (HDF5, TFRecord, etc.)
- Optional: Automated labeling/annotation

**Rationale**: Intrinsic needs "high-quality, training-ready datasets" per role description.

---

### 5. Infrastructure & Deployment ✅

**Architecture**: Multi-container microservices (mirrors Intrinsic's cloud architecture)

**Deployment Environment**:
- **Containers**: Docker-based, orchestrated with Docker Compose (scalable to Kubernetes)
- **Location**: Both containers on same host initially (simulating single workcell)
- **Network**: Bridged network for ROS2 DDS communication

**Storage Infrastructure**:
- **Local Storage**: High-speed SSD for active collection buffer
- **Object Storage**: MinIO (S3-compatible) for long-term storage
- **Database**: PostgreSQL/TimescaleDB for metadata and time-series metrics

**Rationale**: Demonstrates understanding of cloud microservice architectures (Kubernetes, Docker) as required by role.

---

### 6. Data Lifecycle & Distribution ✅

**Retention Policy**:
- **Raw Data**: 30 days in hot storage, then archive or delete
- **Processed Datasets**: Indefinite retention in object storage
- **Metadata**: Permanent retention in database

**Distribution**:
- ✅ REST API for dataset querying and downloading
- ✅ Streaming support for real-time monitoring
- ✅ Export to standard ML formats (HDF5, Zarr, WebDataset)

**Monitoring**:
- Real-time dashboard for collection status
- Data quality metrics visualization
- Alerting for failures or anomalies

**Rationale**: Shows ability to "orchestrate data acquisition from robot workcells" and build internal tools.

---

## Demonstration Value for Interview

### Skills Showcased

1. **Infrastructure Development** ✅
   - Scalable multi-container architecture
   - Cloud-native design patterns
   - Data pipeline orchestration

2. **Data Collection Workflows** ✅
   - ROS2 integration for robot data
   - Sensor synchronization
   - Episode management

3. **Full-Stack Engineering** ✅
   - Web dashboard (TypeScript/React)
   - Python backend APIs
   - Database design

4. **ML Pipeline Integration** ✅
   - Training-ready dataset generation
   - Data quality validation
   - Format conversion for popular frameworks

5. **Production Best Practices** ✅
   - Containerization and orchestration
   - Monitoring and observability
   - API design

### Alignment with AI Challenge 2026
- Demonstrates ability to work with Flowstate-like environments
- Shows understanding of perception data needs (similar to IVM training)
- Proves capability in building infrastructure for "dexterous manipulation" data collection

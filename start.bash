#!/bin/bash

# ==============================================================================
# Vision Sorter - Global Startup Script
# ==============================================================================
# Sequence: Cloud Infrastructure -> Health Check -> Edge Workcells
# ==============================================================================

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Starting Vision Sorter Infrastructure ===${NC}"

# 1. Start cloud services (Storage, DB, API)
echo -e "${YELLOW}Step 1: Launching Cloud Services (MinIO, TimescaleDB, pgAdmin)...${NC}"
docker compose -f docker-compose.cloud.yml up -d

# 2. Wait for cloud services to be healthy
echo -ne "${YELLOW}Step 2: Waiting for databases to be ready${NC}"
MAX_RETRIES=60
RETRY_COUNT=0
while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if docker inspect --format='{{json .State.Health.Status}}' cloud-timescaledb 2>/dev/null | grep -q '"healthy"'; then
        echo -e "\n${GREEN}✓ TimescaleDB is healthy!${NC}"
        break
    fi
    echo -n "."
    sleep 2
    RETRY_COUNT=$((RETRY_COUNT+1))
    if [ $RETRY_COUNT -eq $MAX_RETRIES ]; then
        echo -e "\n${RED}Error: TimescaleDB failed to reach healthy state.${NC}"
        exit 1
    fi
done

# 3. Start edge services - cell 1 and cell 2
echo -e "${YELLOW}Step 3: Launching Edge Workcells...${NC}"

echo "Launching Cell 1..."
./cell_1.bash

echo "Launching Cell 2..."
./cell_2.bash

echo -e "${GREEN}=== All systems launched successfully ===${NC}"
echo -e "Access VNC at:  Cell 1: localhost:5901 | Cell 2: localhost:5902"
echo -e "Access Cloud:   Dashboard: http://localhost:3000 | pgAdmin: http://localhost:5050"
echo -e "Check status:   docker ps"

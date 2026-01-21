#!/bin/bash

# ==============================================================================
# Vision Sorter - Global Shutdown Script
# ==============================================================================
# Sequence: Edge Workcells -> Cloud Infrastructure
# ==============================================================================

RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Stopping Vision Sorter System ===${NC}"

# 1. Stop Edge Workcells
echo -e "${YELLOW}Step 1: Stopping Edge Workcells...${NC}"

echo "Shutting down Cell 1..."
COMPOSE_PROJECT_NAME=cell_1 docker compose -f docker-compose.edge.yml down

echo "Shutting down Cell 2..."
COMPOSE_PROJECT_NAME=cell_2 docker compose -f docker-compose.edge.yml down

# 2. Stop Cloud Infrastructure
echo -e "${YELLOW}Step 2: Stopping Cloud Infrastructure...${NC}"
docker compose -f docker-compose.cloud.yml down

echo -e "${GREEN}=== All systems stopped successfully ===${NC}"
echo -e "You can verify with: docker ps -a"

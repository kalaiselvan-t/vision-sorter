# Use a lightweight Python base image
FROM python:3.10-slim

# Set working directory
WORKDIR /app

# Install system dependencies (needed for h5py and psycopg2)
RUN apt-get update && apt-get install -y \
    libpq-dev \
    gcc \
    libhdf5-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements and install python dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy the application code
COPY ./app /app/app

# Set environment variables for DB and MinIO connection
ENV DB_HOST=localhost
ENV DB_PORT=5432
ENV DB_NAME=robotdata
ENV DB_USER=datauser
ENV DB_PASS=datapass123
ENV MINIO_ENDPOINT=localhost:9000
ENV MINIO_ACCESS_KEY=minioadmin
ENV MINIO_SECRET_KEY=minioadmin123

# Expose the API port
EXPOSE 8000

# Start the FastAPI server
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]

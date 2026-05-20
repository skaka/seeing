FROM ubuntu:24.04

# Avoid interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies needed for compiling Qt6 projects, GUI runtime, and Python AI server
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    libgl1-mesa-dev \
    x11-apps \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install PyTorch, Transformers, FastAPI and other packages needed for Marlin-2B
RUN pip install --break-system-packages \
    torch \
    torchvision \
    torchcodec \
    av \
    pillow \
    qwen-vl-utils \
    accelerate \
    fastapi \
    uvicorn \
    transformers

# Pre-download Qwen/Qwen2-VL-2B-Instruct model weights so it's baked into the image
RUN python3 -c "from transformers import Qwen2VLForConditionalGeneration, AutoProcessor; model = Qwen2VLForConditionalGeneration.from_pretrained('Qwen/Qwen2-VL-2B-Instruct'); processor = AutoProcessor.from_pretrained('Qwen/Qwen2-VL-2B-Instruct'); model.save_pretrained('/app/model_weights'); processor.save_pretrained('/app/model_weights')"

# Set working directory
WORKDIR /app

# Copy project files
COPY . .

# Build the project using CMake
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release -j$(nproc)

# Command to run the Python VLM server in background and compiled binary
CMD python3 src/ai/marlin_server.py & ./build/Seeing

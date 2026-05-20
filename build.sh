#!/usr/bin/env bash
###############################################################################
# @file   build.sh
# @brief  Convenience script to build and run Seeing NLE inside Podman.
###############################################################################

set -euo pipefail

echo "============================================"
echo "  1. Building Podman Image (seeing-nle)...  "
echo "============================================"

# Build the container image
podman build -t seeing-nle .

echo ""
echo "============================================"
echo "  2. Running Seeing NLE Container...        "
echo "============================================"

# Ensure the media folder, Hugging Face cache, and application config directories exist on the host
mkdir -p /home/skaka/other_pro/seeing/media
mkdir -p "$HOME/.cache/huggingface"
mkdir -p "$HOME/.config/SeeingTeam"

# Allow local connections to X11 display
xhost +local:root

# Run the container with GUI forwarding and mounting directories
podman run -it --rm \
  --net=host \
  --ipc=host \
  -e DISPLAY="$DISPLAY" \
  -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
  -v "$HOME/.Xauthority:/root/.Xauthority:ro" \
  -v /home/skaka/other_pro/seeing/media:/media \
  -v "$HOME/.cache/huggingface:/root/.cache/huggingface" \
  -v "$HOME/.config/SeeingTeam:/root/.config/SeeingTeam" \
  localhost/seeing-nle:latest

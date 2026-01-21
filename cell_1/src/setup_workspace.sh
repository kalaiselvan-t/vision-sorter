#!/bin/bash

# This script configures the X server and VNC if the environment variables are set.

if [ -n "$DISPLAY" ] && [ -n "$VNC_PORT" ]; then
    echo "Starting Xvfb on $DISPLAY..."
    # Start Xvfb in the background
    # -screen 0 1280x1024x24: Sets screen 0 to 1280x1024 resolution with 24-bit color
    Xvfb $DISPLAY -screen 0 1280x1024x24 &
    XVFB_PID=$!
    
    # Wait a moment for Xvfb to start
    sleep 2

    # Check if Xvfb is running
    if ! kill -0 $XVFB_PID 2>/dev/null; then
        echo "Error: Xvfb failed to start."
        exit 1
    fi

    echo "Starting Fluxbox window manager..."
    fluxbox &

    echo "Starting x11vnc on port $VNC_PORT..."
    # -forever: Keep listening after client disconnects
    # -shared: Allow multiple viewers
    # -bg: Run in background
    # -display: The display to share
    x11vnc -forever -shared -bg -display $DISPLAY -rfbport $VNC_PORT -nopw

    echo "VNC Setup complete. connect to port $VNC_PORT"
else
    echo "DISPLAY or VNC_PORT not set. Skipping VNC setup."
fi
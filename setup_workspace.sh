#!/bin/bash

# VNC Setup Script - Robust version with GPU rendering support

if [ -n "$DISPLAY" ] && [ -n "$VNC_PORT" ]; then
    echo "=== Starting VNC Setup ==="
    
    # Extract display number from DISPLAY (e.g., :99 -> 99)
    DISPLAY_NUM=$(echo $DISPLAY | sed 's/://')
    SOCKET_FILE="/tmp/.X11-unix/X$DISPLAY_NUM"
    
    echo "Starting Xvfb on $DISPLAY..."
    # Start Xvfb with proper settings for GPU rendering
    Xvfb $DISPLAY -screen 0 1920x1080x24 +extension GLX +render -noreset &
    XVFB_PID=$!
    
    # Wait for X11 socket to be created
    echo "Waiting for X11 socket $SOCKET_FILE..."
    for i in {1..30}; do
        if [ -S "$SOCKET_FILE" ]; then
            echo "✓ X11 socket found"
            break
        fi
        if [ $i -eq 30 ]; then
            echo "✗ Timeout waiting for X11 socket"
            exit 1
        fi
        sleep 0.5
    done
    
    # Verify Xvfb is still running
    if ! kill -0 $XVFB_PID 2>/dev/null; then
        echo "✗ Xvfb failed to start"
        exit 1
    fi
    
    echo "Starting Fluxbox window manager..."
    
    # Create basic Fluxbox configuration for better usability
    mkdir -p /home/user/.fluxbox
    
    # Create a proper menu file with workspace support
    cat > /home/user/.fluxbox/menu << 'EOF'
[begin] (Fluxbox)
    [exec] (Terminal) {xterm} 
    [exec] (Firefox) {firefox}
    [submenu] (Workspaces)
        [workspaces]
    [end]
    [submenu] (System)
        [exec] (Top) {xterm -e top}
        [commanddialog] (Run)
        [reconfig] (Reload config)
        [restart] (Restart)
        [exit] (Exit)
    [end]
[end]
EOF
    
    # Create init file with workspace settings
    cat > /home/user/.fluxbox/init << 'EOF'
session.screen0.workspaces: 4
session.screen0.workspaceNames: Workspace 1,Workspace 2,Workspace 3,Workspace 4
session.screen0.toolbar.visible: true
session.screen0.slit.placement: RightBottom
session.screen0.tab.placement: TopLeft
session.screen0.focusModel: ClickFocus
session.screen0.windowPlacement: RowMinOverlapPlacement
EOF
    
    # Start Fluxbox
    DISPLAY=$DISPLAY fluxbox > /tmp/fluxbox_${DISPLAY_NUM}.log 2>&1 &
    FLUXBOX_PID=$!
    sleep 2
    
    if ! kill -0 $FLUXBOX_PID 2>/dev/null; then
        echo "⚠ Fluxbox may have issues - check /tmp/fluxbox_${DISPLAY_NUM}.log"
    else
        echo "✓ Fluxbox window manager started"
    fi
    
    echo "Starting x11vnc on port $VNC_PORT..."
    # Use setsid to properly daemonize x11vnc so it survives entrypoint exit
    setsid x11vnc \
        -display $DISPLAY \
        -rfbport $VNC_PORT \
        -shared \
        -forever \
        -nopw \
        -xkb \
        -bg \
        -o /tmp/x11vnc_${DISPLAY_NUM}.log \
        > /tmp/x11vnc_${DISPLAY_NUM}_startup.log 2>&1
    
    # Give x11vnc time to daemonize
    sleep 3
    
    # Check if x11vnc is listening
    if netstat -tuln | grep -q ":$VNC_PORT "; then
        echo "✓ VNC Setup complete - connect to localhost:$VNC_PORT"
    else
        echo "⚠ x11vnc may not be listening - check /tmp/x11vnc_${DISPLAY_NUM}_startup.log"
    fi
    
else
    echo "DISPLAY or VNC_PORT not set. Skipping VNC setup."
fi

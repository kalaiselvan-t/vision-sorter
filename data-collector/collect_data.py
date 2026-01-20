#!/usr/bin/env python3
"""
Simple test script to control data collection episodes
"""

import rclpy
from rclpy.node import Node
import sys
import time

# Import the data collector node
from collector_node import DataCollectorNode

def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python3 test_collector.py start [task_type]")
        print("  python3 test_collector.py stop [success]")
        print("")
        print("Examples:")
        print("  python3 test_collector.py start pick_place")
        print("  python3 test_collector.py stop true")
        return
    
    command = sys.argv[1].lower()
    
    rclpy.init()
    node = DataCollectorNode()
    
    try:
        if command == "start":
            task_type = sys.argv[2] if len(sys.argv) > 2 else "test"
            success = node.start_episode(task_type)
            if success:
                print(f"\n✓ Episode started successfully!")
                print(f"  Task: {task_type}")
                print(f"  Episode name: {node.episode_name}")
                print(f"\n  Collecting data... Press Ctrl+C to stop or run:")
                print(f"  python3 test_collector.py stop\n")
                
                try:
                    rclpy.spin(node)
                except KeyboardInterrupt:
                    print("\nStopping episode...")
                    if node.collecting:
                        node.stop_episode(success=True)
            else:
                print("Failed to start episode!")
            
        elif command == "stop":
            success = sys.argv[2].lower() == "true" if len(sys.argv) > 2 else None
            node.stop_episode(success=success)
            print("Episode stopped")
        
        else:
            print(f"Unknown command: {command}")
    
    finally:
        # Clean shutdown - only if context is still valid
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()

if __name__ == '__main__':
    main()

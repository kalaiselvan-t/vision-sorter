#!/usr/bin/env python3
"""
Multi-cube spawner for Gazebo Ignition.
Spawns multiple cubes at random positions on the table with random colors.
"""

import subprocess
import sys
import time
import random
import yaml
import os


# Color definitions (6 standard colors)
COLORS = {
    'red': (1.0, 0.0, 0.0),
    'green': (0.0, 1.0, 0.0),
    'blue': (0.0, 0.0, 1.0),
    'yellow': (1.0, 1.0, 0.0),
    'orange': (1.0, 0.5, 0.0),
    'purple': (0.5, 0.0, 0.5),
}


def run_command(cmd, timeout=5):
    """Execute a shell command and return result."""
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            text=True,
            timeout=timeout
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        print(f"  Command timed out after {timeout}s")
        return -1, "", "Timeout"
    except Exception as e:
        print(f"  Command failed with exception: {e}")
        return -1, "", str(e)


def read_table_config(config_path):
    """
    Read table configuration from planning_scene.yaml.
    Returns table bounds for cube spawning.
    """
    print(f"Reading table config from: {config_path}")

    try:
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)

        # Extract work_table configuration
        work_table = None
        for obj in config['planning_scene']['world']['collision_objects']:
            if obj['id'] == 'work_table':
                work_table = obj
                break

        if not work_table:
            raise ValueError("work_table not found in config")

        # Extract dimensions and position
        dims = work_table['dimensions']  # [x, y, z]
        pos = work_table['pose']['position']  # {x, y, z}

        # Calculate bounds with 5cm margin
        MARGIN = 0.05
        x_min = pos['x'] - dims[0]/2 + MARGIN
        x_max = pos['x'] + dims[0]/2 - MARGIN
        y_min = pos['y'] - dims[1]/2 + MARGIN
        y_max = pos['y'] + dims[1]/2 - MARGIN
        z_spawn = pos['z'] + dims[2]/2 + 0.015  # Table top + half cube height

        bounds = {
            'x_min': x_min,
            'x_max': x_max,
            'y_min': y_min,
            'y_max': y_max,
            'z_spawn': z_spawn
        }

        print(f"  Table bounds: x=[{x_min:.3f}, {x_max:.3f}], y=[{y_min:.3f}, {y_max:.3f}], z={z_spawn:.3f}")
        return bounds

    except Exception as e:
        print(f"  Error reading config: {e}")
        print(f"  Using default table bounds")
        # Default values as fallback
        return {
            'x_min': 0.40,
            'x_max': 0.80,
            'y_min': -0.20,
            'y_max': 0.20,
            'z_spawn': 0.065
        }


def list_existing_cubes():
    """
    List all existing cube models in the world.
    Returns list of cube names.
    """
    cmd = 'ign model --list'
    returncode, stdout, stderr = run_command(cmd, timeout=3)

    if returncode == 0:
        models = [line.strip() for line in stdout.split('\n') if line.strip()]
        # Filter for cube models (names ending with _cube or _cube_N)
        cubes = [m for m in models if 'cube' in m.lower()]
        return cubes
    return []


def get_existing_cube_positions():
    """
    Get positions of all existing cubes.
    Returns list of (x, y, z) tuples.
    """
    positions = []
    existing_cubes = list_existing_cubes()

    for cube_name in existing_cubes:
        cmd = f'ign model -m {cube_name} -p'
        returncode, stdout, stderr = run_command(cmd, timeout=2)

        if returncode == 0:
            try:
                # Parse pose output: "x y z roll pitch yaw"
                parts = stdout.strip().split()
                if len(parts) >= 3:
                    x, y, z = float(parts[0]), float(parts[1]), float(parts[2])
                    positions.append((x, y, z))
            except:
                pass

    return positions


def generate_random_position(table_bounds, existing_positions, min_distance=0.07):
    """
    Generate random position within table bounds, avoiding existing cubes.
    Returns (x, y, z) or None if no valid position found after max attempts.
    """
    MAX_ATTEMPTS = 100

    for _ in range(MAX_ATTEMPTS):
        x = random.uniform(table_bounds['x_min'], table_bounds['x_max'])
        y = random.uniform(table_bounds['y_min'], table_bounds['y_max'])
        z = table_bounds['z_spawn']

        # Check distance from all existing positions
        valid = True
        for ex_x, ex_y, ex_z in existing_positions:
            distance = ((x - ex_x)**2 + (y - ex_y)**2)**0.5
            if distance < min_distance:
                valid = False
                break

        if valid:
            return (x, y, z)

    return None


def generate_cube_name(color, existing_cubes):
    """
    Generate unique cube name with random suffix.
    Format: color_cube_XXXX where XXXX is a random 4-character alphanumeric string.
    """
    import string
    MAX_ATTEMPTS = 100

    for _ in range(MAX_ATTEMPTS):
        # Generate random 4-character suffix
        suffix = ''.join(random.choices(string.ascii_lowercase + string.digits, k=4))
        name = f"{color}_cube_{suffix}"

        if name not in existing_cubes:
            return name

    # Fallback: use timestamp if somehow all random attempts failed
    import time
    timestamp = int(time.time() * 1000) % 100000
    return f"{color}_cube_{timestamp}"


def spawn_cube(name, x, y, z, color_name):
    """Spawn a cube with given name, position, and color."""

    r, g, b = COLORS.get(color_name.lower(), (0.0, 1.0, 0.0))

    print(f"  Spawning {color_name} cube '{name}' at ({x:.3f}, {y:.3f}, {z:.3f})...")

    # Build SDF string
    sdf = (
        '<?xml version="1.0"?>'
        '<sdf version="1.7">'
        f'<model name="{name}">'
        f'<pose>{x} {y} {z} 0 0 0</pose>'
        '<static>false</static>'
        '<link name="cube_link">'
        '<inertial>'
        '<mass>0.1</mass>'
        '<inertia>'
        '<ixx>0.000015</ixx>'
        '<iyy>0.000015</iyy>'
        '<izz>0.000015</izz>'
        '</inertia>'
        '</inertial>'
        '<collision name="collision">'
        '<geometry>'
        '<box><size>0.03 0.03 0.03</size></box>'
        '</geometry>'
        '<surface>'
        '<friction>'
        '<ode><mu>1.5</mu><mu2>1.5</mu2></ode>'
        '</friction>'
        '<contact>'
        '<ode><kp>1000000</kp><kd>100</kd></ode>'
        '</contact>'
        '</surface>'
        '</collision>'
        '<visual name="visual">'
        '<geometry>'
        '<box><size>0.03 0.03 0.03</size></box>'
        '</geometry>'
        '<material>'
        f'<ambient>{r} {g} {b} 1</ambient>'
        f'<diffuse>{r} {g} {b} 1</diffuse>'
        '<specular>0.1 0.1 0.1 1</specular>'
        '</material>'
        '</visual>'
        '</link>'
        '<plugin filename="gz-sim-pose-publisher-system" name="gz::sim::systems::PosePublisher">'
        '<publish_nested_model_pose>true</publish_nested_model_pose>'
        '<publish_link_pose>false</publish_link_pose>'
        '<publish_collision_pose>false</publish_collision_pose>'
        '<publish_visual_pose>false</publish_visual_pose>'
        '<use_pose_vector_msg>false</use_pose_vector_msg>'
        '<static_publisher>false</static_publisher>'
        '<static_update_frequency>10</static_update_frequency>'
        '</plugin>'
        '</model>'
        '</sdf>'
    )

    # Escape for shell
    sdf_escaped = sdf.replace('"', '\\"')

    # Build command
    cmd = (
        f'ign service -s /world/pick_place_world_with_bins/create '
        f'--reqtype ignition.msgs.EntityFactory '
        f'--reptype ignition.msgs.Boolean '
        f'--timeout 5000 '
        f'--req \'sdf: "{sdf_escaped}"\''
    )

    returncode, stdout, stderr = run_command(cmd, timeout=6)

    if returncode == 0 and 'data: true' in stdout:
        print(f"    ✓ Success")
        time.sleep(0.2)  # Brief pause between spawns
        return True
    else:
        print(f"    ✗ Failed")
        if stderr:
            print(f"    Error: {stderr}")
        return False


def main():
    """Main execution function."""

    # Parse arguments
    if len(sys.argv) < 2:
        print("Usage: python3 spawn_multiple_cubes.py <num_cubes>")
        print()
        print("Arguments:")
        print("  num_cubes : Number of cubes to spawn (e.g., 5)")
        print()
        print("Example:")
        print("  python3 spawn_multiple_cubes.py 5")
        print()
        print("Features:")
        print("  - Random colors: red, green, blue, yellow, orange, purple")
        print("  - Random positions on table with 7cm spacing")
        print("  - 5cm margin from table edges")
        print("  - Adds to existing cubes (doesn't clear)")
        sys.exit(1)

    try:
        num_cubes = int(sys.argv[1])
        if num_cubes < 1:
            print("Error: num_cubes must be at least 1")
            sys.exit(1)
    except ValueError as e:
        print(f"Error: Invalid number - {e}")
        sys.exit(1)

    print("=" * 70)
    print("MULTI-CUBE SPAWNER")
    print("=" * 70)
    print(f"Spawning {num_cubes} cube(s) with random colors and positions")
    print()

    # Step 1: Read table configuration
    print("[STEP 1/4] Reading table configuration...")
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = os.path.join(script_dir, '../config/planning_scene.yaml')
    table_bounds = read_table_config(config_path)
    print()

    # Step 2: Get existing cubes and their positions
    print("[STEP 2/4] Checking existing cubes...")
    existing_cubes = list_existing_cubes()
    existing_positions = get_existing_cube_positions()
    print(f"  Found {len(existing_cubes)} existing cube(s)")
    if existing_cubes:
        for cube in existing_cubes:
            print(f"    - {cube}")
    print()

    # Step 3: Generate spawn positions and colors
    print("[STEP 3/4] Generating spawn positions...")
    spawn_plan = []
    positions_taken = existing_positions.copy()

    for i in range(num_cubes):
        # Generate random position
        position = generate_random_position(table_bounds, positions_taken, min_distance=0.07)
        if position is None:
            print(f"  Warning: Could not find valid position for cube {i+1}/{num_cubes}")
            print(f"  Only {i} cube(s) will be spawned")
            break

        # Select random color
        color = random.choice(list(COLORS.keys()))

        # Generate unique name
        name = generate_cube_name(color, existing_cubes)
        existing_cubes.append(name)  # Add to list to avoid duplicates

        spawn_plan.append({
            'name': name,
            'color': color,
            'position': position
        })
        positions_taken.append(position)

        print(f"  {i+1}. {name} at ({position[0]:.3f}, {position[1]:.3f}, {position[2]:.3f})")

    print()

    # Step 4: Spawn cubes
    print(f"[STEP 4/4] Spawning {len(spawn_plan)} cube(s)...")
    success_count = 0

    for i, plan in enumerate(spawn_plan):
        print(f"{i+1}/{len(spawn_plan)}: ", end='')
        if spawn_cube(plan['name'], *plan['position'], plan['color']):
            success_count += 1

    # Final status
    print()
    print("=" * 70)
    if success_count == len(spawn_plan):
        print(f"✓ SUCCESS: All {success_count} cube(s) spawned successfully")
    else:
        print(f"⚠ PARTIAL SUCCESS: {success_count}/{len(spawn_plan)} cube(s) spawned")
    print("=" * 70)

    sys.exit(0 if success_count > 0 else 1)


if __name__ == '__main__':
    main()

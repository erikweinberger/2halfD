
# 2halfD

## Overview

2halfD is a lightweight engine and demo project for exploring 2.5D game development concepts. It provides a foundation for rendering, input handling, and level management, making it easy for users to experiment with classic game mechanics and engine architecture.

## Getting Started

### Prerequisites

- **CMake** (version 3.31.6 or higher)
- **Make** (build tool)
- (Optional) **SFML** and other dependencies are included in the repository.

### Build Instructions

1. Clone the project:
	```
	git clone <repo-url>
	```

2. Change into the project directory:
	```
	cd 2halfD
	```

3. Create a build directory and enter it:
	```
	mkdir build
	cd build
	```

4. Run CMake to configure the project:
	```
	cmake ..
	```

5. Build the project:
	```
	make
	```

6. Run the demo:
	```
	./two_half
	```

## BSP Visualizer

A debug tool (`bsp_vis`) that renders the BSP graph for any level as a 2D top-down view. Useful for inspecting region adjacency, graph edges, and pathfinding waypoints.

### Building

After the initial `cmake ..` + `make`, the visualizer is built alongside the main game. To build it explicitly:

```
cmake -B build .
cmake --build build --target bsp_vis
```

### Running

```
./build/bsp_vis
```

By default it loads `assets/levels/level3.txt`. Change the path in `game/visualizer.cpp` to visualise a different level.

### Controls

| Input | Action |
|---|---|
| Scroll wheel | Zoom in / out |
| Middle-mouse drag | Pan |
| Escape | Quit |

### Legend

| Colour | What it represents |
|---|---|
| Dark grey lines | Convex region outlines — the full extent of each BSP split, including where the splitter line extends beyond the actual wall/floor geometry |
| White lines | Solid walls |
| Light grey lines | Floor section boundaries |
| Yellow dots | Node centroids — one per convex region, used as the start/end waypoint for that region during pathfinding |
| Green dots | Portal midpoints — the midpoint of each shared edge between two adjacent regions; these are the intermediate waypoints agents step through |
| Blue lines | `centroid → portal → centroid` routing for bidirectional edges (agent can walk either direction) |
| Orange lines | Same routing but for drop-only edges (agent can only fall from the higher region down to the lower one) |

### Pathfinding note

Routing through portal midpoints (green dots) rather than directly between centroids (yellow dots) guarantees the path never clips through a wall. Each leg of the path (`centroid → portal midpoint` and `portal midpoint → centroid`) stays entirely within a single convex cell, so it is always wall-free. The high-level A* search runs on the node graph as normal; the final movement waypoint sequence is: `centroid_A, portal_midpoint(A→B), centroid_B, portal_midpoint(B→C), ...`

## Purpose

This project demonstrates a simple 2.5D engine structure, including:
- Level loading and rendering
- Basic input handling
- BSP tree construction and adjacency graph for pathfinding

Feel free to explore, modify, and use as a starting point for your own 2.5D projects!

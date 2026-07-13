# Maze Generator
Generates a maze and sets it as the obstacle map for the simulation.
## Usage
1. Run the script — a "Maze Generator" panel appears with **Cell size (px)** and **Wall thickness (px)** fields, and a **Generate** button.
2. Adjust the values if needed (defaults: cell size 3px, wall thickness 1px).
3. Click **Generate** to build a new maze and apply it to the running simulation.
## Notes
- The maze is centered in the simulation area; if the simulation size isn't evenly divisible by the cell size, a small margin is left unused along the edges.
- Cell size should be noticeably larger than wall thickness, or a cell's interior can shrink to zero.
- Each click generates a new random maze, replacing the current obstacles.
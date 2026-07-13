import random

import numpy as np

from fluidsim import physics, ui


def generate_maze(cols: int, rows: int):
    visited = [[False] * cols for _ in range(rows)]
    walls_removed: set[frozenset[tuple[int, int]]] = set()

    stack = [(0, 0)]
    visited[0][0] = True

    while stack:
        r, c = stack[-1]
        neighbors = []
        for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nr, nc = r + dr, c + dc
            if 0 <= nr < rows and 0 <= nc < cols and not visited[nr][nc]:
                neighbors.append((nr, nc))

        if not neighbors:
            stack.pop()
            continue

        nr, nc = random.choice(neighbors)
        walls_removed.add(frozenset({(r, c), (nr, nc)}))
        visited[nr][nc] = True
        stack.append((nr, nc))

    return walls_removed


def rasterize_maze(
    sim_w: int,
    sim_h: int,
    cols: int,
    rows: int,
    cell_size: int,
    wall_thickness: int,
    walls_removed: set[frozenset[tuple[int, int]]],
):
    obstacles = np.ones((sim_h, sim_w), dtype=np.uint32)

    maze_w = cols * cell_size
    maze_h = rows * cell_size
    offset_x = (sim_w - maze_w) // 2
    offset_y = (sim_h - maze_h) // 2

    for y in range(rows):
        for x in range(cols):
            y0, x0 = offset_y + y * cell_size, offset_x + x * cell_size
            y1, x1 = min(y0 + cell_size, sim_h), min(x0 + cell_size, sim_w)

            obstacles[y0 + wall_thickness : y1, x0 + wall_thickness : x1] = 0

            if frozenset({(y, x), (y, x + 1)}) in walls_removed:
                obstacles[
                    y0 + wall_thickness : y1, x1 - wall_thickness : x1 + wall_thickness
                ] = 0

            if frozenset({(y, x), (y + 1, x)}) in walls_removed:
                obstacles[
                    y1 - wall_thickness : y1 + wall_thickness, x0 + wall_thickness : x1
                ] = 0

    return obstacles


def on_generate(state):
    sim_w, sim_h = physics.get_sim_size()
    cell_size = state["cell_size"]
    wall_thickness = state["wall_thickness"]

    cols = sim_w // cell_size
    rows = sim_h // cell_size

    walls_removed = generate_maze(cols, rows)
    obstacles = rasterize_maze(
        sim_w, sim_h, cols, rows, cell_size, wall_thickness, walls_removed
    )
    physics.set_obstacles(obstacles)


panel = ui.Panel()
panel.title = "Maze Generator"
panel.add_drag_int("cell_size", "Cell size (px)", 3)
panel.add_drag_int("wall_thickness", "Wall thickness (px)", 1)
panel.add_button("generate", "Generate", on_generate)

ui.set_panel(panel)

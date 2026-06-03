struct Params {
    width: u32,
    height: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(2) var<storage, read_write> divergence: array<f32>;
@group(0) @binding(3) var<storage, read> obstacles: array<u32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.width || y >= params.height { return; }

    let i = idx(x, y);
    if obstacles[i] != 0u { divergence[i] = 0.0; return; }

    let x0 = select(x - 1, x, x == 0);
    let x1 = select(x + 1, x, x == params.width - 1);
    let y0 = select(y - 1, y, y == 0);
    let y1 = select(y + 1, y, y == params.height - 1);

    let vx1 = select(velocity[idx(x1, y)].x, -velocity[i].x, obstacles[idx(x1, y)] != 0u || x1 == x);
    let vx0 = select(velocity[idx(x0, y)].x, -velocity[i].x, obstacles[idx(x0, y)] != 0u || x0 == x);
    let vy1 = select(velocity[idx(x, y1)].y, -velocity[i].y, obstacles[idx(x, y1)] != 0u || y1 == y);
    let vy0 = select(velocity[idx(x, y0)].y, -velocity[i].y, obstacles[idx(x, y0)] != 0u || y0 == y);

    divergence[i] = 0.5 * (vx1 - vx0 + vy1 - vy0);
}

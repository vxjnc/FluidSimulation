struct Params {
    width: u32,
    height: u32,
    dt: f32,
    vel_dissipation: f32,
    dye_dissipation: f32,
    curl_strength: f32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(2) var<storage, read_write> curl: array<f32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.width || y >= params.height { return; }

    let x0 = select(x - 1u, x, x == 0u);
    let x1 = select(x + 1u, x, x == params.width - 1u);
    let y0 = select(y - 1u, y, y == 0u);
    let y1 = select(y + 1u, y, y == params.height - 1u);

    let vR = velocity[idx(x1, y)].y;
    let vL = velocity[idx(x0, y)].y;
    let vT = velocity[idx(x, y1)].x;
    let vB = velocity[idx(x, y0)].x;

    curl[idx(x, y)] = (vR - vL - vT + vB) * 0.5;
}

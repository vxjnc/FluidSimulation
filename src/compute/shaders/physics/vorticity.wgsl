struct Params {
    width: u32,
    height: u32,
    dye_width: u32,
    dye_height: u32,
    dt: f32,
    vel_dissipation: f32,
    dye_dissipation: f32,
    curl_strength: f32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> curl: array<f32>;
@group(0) @binding(2) var<storage, read_write> velocity: array<vec2f>;

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

    let cR = curl[idx(x1, y)];
    let cL = curl[idx(x0, y)];
    let cT = curl[idx(x, y1)];
    let cB = curl[idx(x, y0)];
    let cC = curl[idx(x, y)];

    var force = vec2f(abs(cT) - abs(cB), abs(cR) - abs(cL)) * 0.5;
    let len = length(force) + 0.0001;
    force = (force / len) * cC * params.curl_strength * params.dt;
    force.y *= -1.0;

    velocity[idx(x, y)] = clamp(velocity[idx(x, y)] + force, vec2f(-1000.0), vec2f(1000.0));
}

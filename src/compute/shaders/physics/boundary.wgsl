struct Params {
    width: u32,
    height: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read_write> velocity: array<vec2f>;
@group(0) @binding(2) var<storage, read> obstacles: array<u32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.width || y >= params.height { return; }

    let i = idx(x, y);
    if obstacles[i] != 0u {
        velocity[i] = vec2f(0.0);
        return;
    }

    var v = velocity[i];

    let left_blocked = x > 0u && obstacles[idx(x - 1u, y)] != 0u;
    let right_blocked = x < params.width - 1u && obstacles[idx(x + 1u, y)] != 0u;
    let down_blocked = y > 0u && obstacles[idx(x, y - 1u)] != 0u;
    let up_blocked = y < params.height - 1u && obstacles[idx(x, y + 1u)] != 0u;

    if left_blocked { v.x = max(v.x, 0.0); }
    if right_blocked { v.x = min(v.x, 0.0); }
    if down_blocked { v.y = max(v.y, 0.0); }
    if up_blocked { v.y = min(v.y, 0.0); }

    if x == 0u { v.x = max(v.x, 0.0); }
    if x == params.width - 1u { v.x = min(v.x, 0.0); }
    if y == 0u { v.y = max(v.y, 0.0); }
    if y == params.height - 1u { v.y = min(v.y, 0.0); }

    velocity[i] = v;
}

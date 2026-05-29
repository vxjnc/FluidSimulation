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

    let i = y * params.width + x;
    if obstacles[i] != 0u {
        velocity[i] = vec2f(0.0);
        return;
    }

    let vx_blocked = (x > 0u && obstacles[idx(x - 1u, y)] != 0u) ||
                 (x < params.width - 1u && obstacles[idx(x + 1u, y)] != 0u);
    let vy_blocked = (y > 0u && obstacles[idx(x, y - 1u)] != 0u) ||
                 (y < params.height - 1u && obstacles[idx(x, y + 1u)] != 0u);

    var v = velocity[i];
    if vx_blocked { v.x = 0.0; }
    if vy_blocked { v.y = 0.0; }

    let on_left = x == 0u;
    let on_right = x == params.width - 1u;
    let on_bottom = y == 0u;
    let on_top = y == params.height - 1u;

    if on_left || on_right { v.x = 0.0; }
    if on_top || on_bottom { v.y = 0.0; }

    velocity[i] = v;
}

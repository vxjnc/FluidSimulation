struct Params {
    width: u32,
    height: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<uniform> dt: f32;
@group(0) @binding(2) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(3) var<storage, read_write> velocity_next: array<vec2f>;
@group(0) @binding(4) var<storage, read> obstacles: array<u32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

fn sample_velocity(pos: vec2f) -> vec2f {
    let x = clamp(pos.x, 0.0, f32(params.width - 1));
    let y = clamp(pos.y, 0.0, f32(params.height - 1));

    let x0 = u32(x);
    let y0 = u32(y);
    let x1 = min(x0 + 1, params.width - 1);
    let y1 = min(y0 + 1, params.height - 1);

    let tx = x - f32(x0);
    let ty = y - f32(y0);

    let v00 = velocity[idx(x0, y0)];
    let v10 = velocity[idx(x1, y0)];
    let v01 = velocity[idx(x0, y1)];
    let v11 = velocity[idx(x1, y1)];

    return mix(mix(v00, v10, tx), mix(v01, v11, tx), ty);
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.width || y >= params.height { return; }

    let i = idx(x, y);
    if obstacles[i] != 0u {
        velocity_next[i] = vec2f(0.0);
        return;
    }

    let pos = vec2f(f32(x), f32(y));
    let vel = velocity[i];
    let src_pos = pos - vel * dt;

    const DISSIPATION = 0.1;
    velocity_next[i] = sample_velocity(src_pos) * (1.0 - DISSIPATION * dt);
}

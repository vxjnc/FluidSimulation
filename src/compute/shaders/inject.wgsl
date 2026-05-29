struct Params {
    width: u32,
    height: u32,
}
struct Inject {
    x: f32,
    y: f32,
    vx: f32,
    vy: f32,
    radius: f32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<uniform> inject: Inject;
@group(0) @binding(2) var<storage, read_write> velocity: array<vec2f>;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;

    let d = distance(vec2f(f32(x), f32(y)), vec2f(inject.x, inject.y));
    if d >= inject.radius { return; }

    let falloff = 1.0 - d / inject.radius;
    velocity[y * params.width + x] += vec2f(inject.vx, inject.vy) * falloff;
}

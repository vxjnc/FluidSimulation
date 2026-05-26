@group(0) @binding(0) var<storage, read> fluid: array<vec2f>;

struct VertOut {
    @builtin(position) pos: vec4f,
    @location(0) uv: vec2f,
}

@vertex
fn vs_main(@builtin(vertex_index) vi: u32) -> VertOut {
    var positions = array<vec2f, 4>(
        vec2f(-1.0, -1.0),
        vec2f( 1.0, -1.0),
        vec2f(-1.0,  1.0),
        vec2f( 1.0,  1.0),
    );
    var out: VertOut;
    out.pos = vec4f(positions[vi], 0.0, 1.0);
    out.uv  = positions[vi] * 0.5 + 0.5;
    return out;
}

@group(0) @binding(1) var<uniform> dims: vec2u;
@fragment
fn fs_main(in: VertOut) -> @location(0) vec4f {
    let ix = u32(in.uv.x * f32(dims.x));
    let iy = u32(in.uv.y * f32(dims.y));
    let v  = fluid[iy * dims.x + ix];
    let speed = length(v);
    let dir   = v / speed * 0.5 + 0.5;
    return vec4f(dir, speed, 1.0);
}

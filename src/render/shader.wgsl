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

fn hsv2rgb(h: f32, s: f32, v: f32) -> vec3f {
    let h6 = h * 6.0;
    let i  = floor(h6);
    let f  = h6 - i;
    let p  = v * (1.0 - s);
    let q  = v * (1.0 - s * f);
    let t  = v * (1.0 - s * (1.0 - f));
    switch u32(i) % 6u {
        case 0u: { return vec3f(v, t, p); }
        case 1u: { return vec3f(q, v, p); }
        case 2u: { return vec3f(p, v, t); }
        case 3u: { return vec3f(p, q, v); }
        case 4u: { return vec3f(t, p, v); }
        default: { return vec3f(v, p, q); }
    }
}

@fragment
fn fs_main(in: VertOut) -> @location(0) vec4f {
    let ix  = u32(in.uv.x * f32(dims.x));
    let iy  = u32(in.uv.y * f32(dims.y));
    let v   = fluid[iy * dims.x + ix];
    let speed = length(v);
    let t = clamp(speed / 110.0, 0.0, 1.0);
    // return vec4f(t, t, t, 1.0);
    let hue = (atan2(v.y, v.x) / (2.0 * 3.14159265) + 1.0) % 1.0;
    let val = 1.0 - exp(-speed * 2.0);
    return vec4f(hsv2rgb(hue, 1.0, val), 1.0);
}

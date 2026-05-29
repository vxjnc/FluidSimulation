@group(0) @binding(0) var<storage, read> dye: array<f32>;
@group(0) @binding(1) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(2) var<storage, read> pressure: array<f32>;
@group(0) @binding(3) var<storage, read> divergence: array<f32>;
@group(0) @binding(4) var<uniform> dims: vec4u; // width, height, mode, _

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
    let i = vec2u(in.uv * vec2f(dims.xy));
    let idx = i.y * dims.x + i.x;

    const pi = 3.14159265;

    switch dims.z {
        case 0u: {
            let d = clamp(dye[idx], 0.0, 1.0);
            let v = velocity[idx];
            let hue = (atan2(v.y, v.x) / (2.0 * pi) + 1.0) % 1.0;
            return vec4f(hsv2rgb(hue, 1.0, d), 1.0);
        }
        case 1u: {
            let v = velocity[idx];
            let hue = (atan2(v.y, v.x) / (2.0 * pi) + 1.0) % 1.0;
            let mag = clamp(length(v) * 0.1, 0.0, 1.0);
            return vec4f(hsv2rgb(hue, 1.0, mag), 1.0);
        }
        case 2u: {
            let p = pressure[idx];
            let pos = clamp( p * 0.1, 0.0, 1.0);
            let neg = clamp(-p * 0.1, 0.0, 1.0);
            return vec4f(pos, 0.0, neg, 1.0);
        }
        case 3u: {
            let d = clamp(divergence[idx], 0.0, 1.0);
            let v = velocity[idx];
            let hue = (atan2(v.y, v.x) / (2.0 * pi) + 1.0) % 1.0;
            return vec4f(hsv2rgb(hue, 1.0, d), 1.0);
        }
        default: {
            return vec4f(0.0, 0.0, 0.0, 1.0);
        }
    }
}

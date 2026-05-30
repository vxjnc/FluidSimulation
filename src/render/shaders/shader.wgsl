@group(0) @binding(0) var<storage, read> dye: array<f32>;
@group(0) @binding(1) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(2) var<storage, read> pressure: array<f32>;
@group(0) @binding(3) var<storage, read> divergence: array<f32>;
@group(0) @binding(4) var<storage, read> obstacles: array<u32>;
@group(0) @binding(5) var<uniform> dims: vec4u; // width, height, mode, showObstacles

struct VertOut {
    @builtin(position) pos: vec4f,
    @location(0) uv: vec2f,
}

@vertex
fn vs_main(@builtin(vertex_index) vi: u32) -> VertOut {
    var positions = array<vec2f, 4>(
        vec2f(-1.0, -1.0),
        vec2f(1.0, -1.0),
        vec2f(-1.0, 1.0),
        vec2f(1.0, 1.0),
    );
    var out: VertOut;
    out.pos = vec4f(positions[vi], 0.0, 1.0);
    out.uv = positions[vi] * 0.5 + 0.5;
    return out;
}

fn hsv2rgb(h: f32, s: f32, v: f32) -> vec3f {
    let h6 = h * 6.0;
    let i = floor(h6);
    let f = h6 - i;
    let p = v * (1.0 - s);
    let q = v * (1.0 - s * f);
    let t = v * (1.0 - s * (1.0 - f));
    switch u32(i) % 6u {
        case 0u: { return vec3f(v, t, p); }
        case 1u: { return vec3f(q, v, p); }
        case 2u: { return vec3f(p, v, t); }
        case 3u: { return vec3f(p, q, v); }
        case 4u: { return vec3f(t, p, v); }
        default: { return vec3f(v, p, q); }
    }
}

fn grid_idx(x: u32, y: u32) -> u32 {
    return y * dims.x + x;
}

fn bilinear_coords(uv: vec2f) -> vec4u {
    let pos = clamp(uv * vec2f(dims.xy) - 0.5, vec2f(0.0), vec2f(dims.xy) - vec2f(1.0));
    let xy0 = vec2u(pos);
    let xy1 = min(xy0 + vec2u(1u), dims.xy - vec2u(1u));
    return vec4u(xy0, xy1);
}

fn bilinear_t(uv: vec2f) -> vec2f {
    let pos = clamp(uv * vec2f(dims.xy) - 0.5, vec2f(0.0), vec2f(dims.xy) - vec2f(1.0));
    return fract(pos);
}

fn sample_dye(uv: vec2f) -> f32 {
    let c = bilinear_coords(uv);
    let t = bilinear_t(uv);
    let d00 = dye[grid_idx(c.x, c.y)];
    let d10 = dye[grid_idx(c.z, c.y)];
    let d01 = dye[grid_idx(c.x, c.w)];
    let d11 = dye[grid_idx(c.z, c.w)];
    return mix(mix(d00, d10, t.x), mix(d01, d11, t.x), t.y);
}

fn sample_velocity(uv: vec2f) -> vec2f {
    let c = bilinear_coords(uv);
    let t = bilinear_t(uv);
    let v00 = velocity[grid_idx(c.x, c.y)];
    let v10 = velocity[grid_idx(c.z, c.y)];
    let v01 = velocity[grid_idx(c.x, c.w)];
    let v11 = velocity[grid_idx(c.z, c.w)];
    return mix(mix(v00, v10, t.x), mix(v01, v11, t.x), t.y);
}

fn sample_pressure(uv: vec2f) -> f32 {
    let c = bilinear_coords(uv);
    let t = bilinear_t(uv);
    let p00 = pressure[grid_idx(c.x, c.y)];
    let p10 = pressure[grid_idx(c.z, c.y)];
    let p01 = pressure[grid_idx(c.x, c.w)];
    let p11 = pressure[grid_idx(c.z, c.w)];
    return mix(mix(p00, p10, t.x), mix(p01, p11, t.x), t.y);
}

fn sample_divergence(uv: vec2f) -> f32 {
    let c = bilinear_coords(uv);
    let t = bilinear_t(uv);
    let d00 = divergence[grid_idx(c.x, c.y)];
    let d10 = divergence[grid_idx(c.z, c.y)];
    let d01 = divergence[grid_idx(c.x, c.w)];
    let d11 = divergence[grid_idx(c.z, c.w)];
    return mix(mix(d00, d10, t.x), mix(d01, d11, t.x), t.y);
}

@fragment
fn fs_main(in: VertOut) -> @location(0) vec4f {
    let i = vec2u(in.uv * vec2f(dims.xy));
    let idx = i.y * dims.x + i.x;

    const pi = 3.14159265;

    if dims.w != 0u && obstacles[idx] != 0u {
        return vec4f(1.0);
    }

    var color = vec4f(0.0, 0.0, 0.0, 1.0);
    switch dims.z {
        case 0u: {
            let d = clamp(sample_dye(in.uv), 0.0, 1.0);
            let v = sample_velocity(in.uv);
            let hue = (atan2(v.y, v.x) / (2.0 * pi) + 1.0) % 1.0;
            color = vec4f(hsv2rgb(hue, 1.0, d), 1.0);
        }
        case 1u: {
            let v = sample_velocity(in.uv);
            let hue = (atan2(v.y, v.x) / (2.0 * pi) + 1.0) % 1.0;
            let mag = clamp(length(v) * 0.1, 0.0, 1.0);
            color = vec4f(hsv2rgb(hue, 1.0, mag), 1.0);
        }
        case 2u: {
            let p = sample_pressure(in.uv);
            let pos = clamp(p * 0.1, 0.0, 1.0);
            let neg = clamp(-p * 0.1, 0.0, 1.0);
            color = vec4f(pos, 0.0, neg, 1.0);
        }
        case 3u: {
            let d = clamp(sample_divergence(in.uv), 0.0, 1.0);
            let v = sample_velocity(in.uv);
            let hue = (atan2(v.y, v.x) / (2.0 * pi) + 1.0) % 1.0;
            color = vec4f(hsv2rgb(hue, 1.0, d), 1.0);
        }
        default: {
            color = vec4f(0.0, 0.0, 0.0, 1.0);
        }
    }

    return color;
}

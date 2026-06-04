struct Params {
    width: u32,
    height: u32,
    dye_width: u32,
    dye_height: u32,
    mode: u32,
    show_obstacles: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> dye: array<vec4f>;
@group(0) @binding(2) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(3) var<storage, read> pressure: array<f32>;
@group(0) @binding(4) var<storage, read> divergence: array<f32>;
@group(0) @binding(5) var<storage, read> obstacles: array<u32>;
@group(0) @binding(6) var<storage, read> curl: array<f32>;

struct VertOut {
    @builtin(position) pos: vec4f,
    @location(0) uv: vec2f,
}

@vertex
fn vs_main(@builtin(vertex_index) vi: u32) -> VertOut {
    var positions = array(
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

fn dye_idx(x: u32, y: u32) -> u32 {
    return y * params.dye_width + x;
}

fn phys_idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

fn bilinear_dye(uv: vec2f) -> vec4f {
    let pos = clamp(uv * vec2f(f32(params.dye_width), f32(params.dye_height)) - 0.5,
        vec2f(0.0), vec2f(f32(params.dye_width) - 1.0, f32(params.dye_height) - 1.0));
    let xy0 = vec2u(pos);
    let xy1 = min(xy0 + vec2u(1u), vec2u(params.dye_width - 1u, params.dye_height - 1u));
    let t = fract(pos);
    let d00 = dye[dye_idx(xy0.x, xy0.y)];
    let d10 = dye[dye_idx(xy1.x, xy0.y)];
    let d01 = dye[dye_idx(xy0.x, xy1.y)];
    let d11 = dye[dye_idx(xy1.x, xy1.y)];
    return mix(mix(d00, d10, t.x), mix(d01, d11, t.x), t.y);
}

fn bilinear_phys(uv: vec2f) -> vec4u {
    let pos = clamp(uv * vec2f(f32(params.width), f32(params.height)) - 0.5,
        vec2f(0.0), vec2f(f32(params.width) - 1.0, f32(params.height) - 1.0));
    let xy0 = vec2u(pos);
    let xy1 = min(xy0 + vec2u(1u), vec2u(params.width - 1u, params.height - 1u));
    return vec4u(xy0, xy1);
}

fn bilinear_t_phys(uv: vec2f) -> vec2f {
    let pos = clamp(uv * vec2f(f32(params.width), f32(params.height)) - 0.5,
        vec2f(0.0), vec2f(f32(params.width) - 1.0, f32(params.height) - 1.0));
    return fract(pos);
}

fn sample_velocity(uv: vec2f) -> vec2f {
    let c = bilinear_phys(uv);
    let t = bilinear_t_phys(uv);
    let v00 = velocity[phys_idx(c.x, c.y)];
    let v10 = velocity[phys_idx(c.z, c.y)];
    let v01 = velocity[phys_idx(c.x, c.w)];
    let v11 = velocity[phys_idx(c.z, c.w)];
    return mix(mix(v00, v10, t.x), mix(v01, v11, t.x), t.y);
}

fn sample_pressure(uv: vec2f) -> f32 {
    let c = bilinear_phys(uv);
    let t = bilinear_t_phys(uv);
    let p00 = pressure[phys_idx(c.x, c.y)];
    let p10 = pressure[phys_idx(c.z, c.y)];
    let p01 = pressure[phys_idx(c.x, c.w)];
    let p11 = pressure[phys_idx(c.z, c.w)];
    return mix(mix(p00, p10, t.x), mix(p01, p11, t.x), t.y);
}

fn sample_divergence(uv: vec2f) -> f32 {
    let c = bilinear_phys(uv);
    let t = bilinear_t_phys(uv);
    let d00 = divergence[phys_idx(c.x, c.y)];
    let d10 = divergence[phys_idx(c.z, c.y)];
    let d01 = divergence[phys_idx(c.x, c.w)];
    let d11 = divergence[phys_idx(c.z, c.w)];
    return mix(mix(d00, d10, t.x), mix(d01, d11, t.x), t.y);
}

@fragment
fn fs_main(in: VertOut) -> @location(0) vec4f {
    let phys_i = vec2u(in.uv * vec2f(f32(params.width), f32(params.height)));
    let obs_idx = phys_i.y * params.width + phys_i.x;

    const pi = 3.14159265;

    if params.show_obstacles != 0u && obstacles[obs_idx] != 0u {
        return vec4f(1.0);
    }

    var color = vec4f(0.0, 0.0, 0.0, 1.0);
    switch params.mode {
        case 0u: {
            let d = bilinear_dye(in.uv);
            color = vec4f(d.rgb, 1.0);
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
        case 4u: {
            let c = bilinear_phys(in.uv);
            let t = bilinear_t_phys(in.uv);
            let c00 = curl[phys_idx(c.x, c.y)];
            let c10 = curl[phys_idx(c.z, c.y)];
            let c01 = curl[phys_idx(c.x, c.w)];
            let c11 = curl[phys_idx(c.z, c.w)];
            let val = mix(mix(c00, c10, t.x), mix(c01, c11, t.x), t.y);
            let pos = clamp(val * 0.1, 0.0, 1.0);
            let neg = clamp(-val * 0.1, 0.0, 1.0);
            color = vec4f(pos, 0.0, neg, 1.0);
        }
        default: {
            color = vec4f(0.0, 0.0, 0.0, 1.0);
        }
    }

    return color;
}

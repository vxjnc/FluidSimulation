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

struct Inject {
    color: vec4f,
    x: f32,
    y: f32,
    vx: f32,
    vy: f32,
    radius: f32,
    mode_mask: u32,
    form: u32,
    _pad: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<uniform> inject_count: u32;
@group(0) @binding(2) var<storage, read> injects: array<Inject>;
@group(0) @binding(3) var<storage, read_write> velocity: array<vec2f>;
@group(0) @binding(4) var<storage, read_write> dye: array<vec4f>;

fn idx_phys(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

fn idx_dye(x: u32, y: u32) -> u32 {
    return y * params.dye_width + x;
}

fn distance_to_segment(p: vec2f, a: vec2f, b: vec2f) -> f32 {
    let ba = b - a;
    let pa = p - a;
    let h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

fn calc_distance(cell: vec2f, src: vec2f, radius: f32, inject: Inject) -> f32 {
    if inject.form == 0u {
        return distance(cell, src);
    }
    let vel = vec2f(inject.vx, inject.vy);
    var line_dir = vec2f(1.0, 0.0);
    if length(vel) > 0.0 {
        line_dir = normalize(vec2f(-vel.y, vel.x));
    }
    let p0 = src - line_dir * radius;
    let p1 = src + line_dir * radius;
    return distance_to_segment(cell, p0, p1);
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;

    let in_phys = x < params.width && y < params.height;
    let in_dye = x < params.dye_width && y < params.dye_height;
    if !in_phys && !in_dye { return; }

    for (var i = 0u; i < inject_count; i++) {
        let inj = injects[i];

        // --- velocity ---
        if in_phys && (inj.mode_mask & 1u) != 0u {
            let phys_cell = vec2f(f32(x), f32(y));
            let src_phys = vec2f(inj.x * f32(params.width), inj.y * f32(params.height));
            let radius_phys = inj.radius * f32(params.height);
            let eff_r = select(radius_phys, 2.0, inj.form == 1u);
            let d = calc_distance(phys_cell, src_phys, radius_phys, inj);
            if d < eff_r {
                let falloff = 1.0 - d / eff_r;
                let vel_phys = vec2f(inj.vx, inj.vy) * f32(params.height) * params.dt;
                velocity[idx_phys(x, y)] += vel_phys * falloff;
            }
        }

        // --- dye ---
        if in_dye && (inj.mode_mask & 2u) != 0u {
            let dye_cell = vec2f(f32(x), f32(y));
            let src_dye = vec2f(inj.x * f32(params.dye_width), inj.y * f32(params.dye_height));
            let radius_dye = inj.radius * f32(params.dye_height);
            let eff_r = select(radius_dye, 2.0, inj.form == 1u);
            let d = calc_distance(dye_cell, src_dye, radius_dye, inj);
            if d < eff_r {
                let falloff = 1.0 - d / eff_r;
                dye[idx_dye(x, y)] = min(dye[idx_dye(x, y)] + vec4f(inj.color.rgb * falloff, falloff), vec4f(1.0));
            }
        }
    }
}

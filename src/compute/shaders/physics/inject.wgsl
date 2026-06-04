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
    mode_mask: u32,
    form: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<uniform> inject: Inject;
@group(0) @binding(2) var<storage, read_write> velocity: array<vec2f>;
@group(0) @binding(3) var<storage, read_write> dye: array<f32>;

fn distance_to_segment(p: vec2f, a: vec2f, b: vec2f) -> f32 {
    let ba = b - a;
    let pa = p - a;
    let h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;

    if x >= params.width || y >= params.height { return; }

    let cell_pos = vec2f(f32(x), f32(y));
    var d: f32 = 0.0;

    // Form::CIRCLE
    if inject.form == 0u {
        d = distance(cell_pos, vec2f(inject.x, inject.y));
    }
    // Form::LINE
    else {
        let center = vec2f(inject.x, inject.y);
        let vel = vec2f(inject.vx, inject.vy);

        var line_dir = vec2f(1.0, 0.0);
        if length(vel) > 0.0 {
            line_dir = normalize(vec2f(-vel.y, vel.x));
        }

        let p0 = center - line_dir * inject.radius;
        let p1 = center + line_dir * inject.radius;

        d = distance_to_segment(cell_pos, p0, p1);
    }

    let effective_radius = select(inject.radius, 2.0, inject.form == 1u);

    if d >= effective_radius { return; }

    let falloff = 1.0 - d / effective_radius;
    let index = y * params.width + x;

    // Mode::VELOCITY
    if (inject.mode_mask & 1u) != 0u {
        velocity[index] += vec2f(inject.vx, inject.vy) * falloff;
    }

    // Mode::DYE
    if (inject.mode_mask & 2u) != 0u {
        dye[index] = min(dye[index] + falloff, 1.0);
    }
}

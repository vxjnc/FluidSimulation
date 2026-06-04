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

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(2) var<storage, read> dye: array<vec4f>;
@group(0) @binding(3) var<storage, read_write> dye_next: array<vec4f>;
@group(0) @binding(4) var<storage, read> obstacles: array<u32>;

fn dye_idx(x: u32, y: u32) -> u32 {
    return y * params.dye_width + x;
}

fn vel_idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

fn sample_velocity(dye_pos: vec2f) -> vec2f {
    let phys_pos = dye_pos * vec2f(f32(params.width), f32(params.height)) / vec2f(f32(params.dye_width), f32(params.dye_height));

    let x = clamp(phys_pos.x, 0.0, f32(params.width - 1));
    let y = clamp(phys_pos.y, 0.0, f32(params.height - 1));
    let x0 = u32(x);
    let y0 = u32(y);
    let x1 = min(x0 + 1, params.width - 1);
    let y1 = min(y0 + 1, params.height - 1);
    let tx = x - f32(x0);
    let ty = y - f32(y0);

    let v00 = velocity[vel_idx(x0, y0)];
    let v10 = velocity[vel_idx(x1, y0)];
    let v01 = velocity[vel_idx(x0, y1)];
    let v11 = velocity[vel_idx(x1, y1)];
    return mix(mix(v00, v10, tx), mix(v01, v11, tx), ty);
}

fn sample_dye(pos: vec2f) -> vec4f {
    let x = clamp(pos.x, 0.0, f32(params.dye_width - 1));
    let y = clamp(pos.y, 0.0, f32(params.dye_height - 1));
    let x0 = u32(x); let y0 = u32(y);
    let x1 = min(x0 + 1, params.dye_width - 1);
    let y1 = min(y0 + 1, params.dye_height - 1);
    let tx = x - f32(x0);
    let ty = y - f32(y0);

    let v00 = dye[dye_idx(x0, y0)];
    let v10 = dye[dye_idx(x1, y0)];
    let v01 = dye[dye_idx(x0, y1)];
    let v11 = dye[dye_idx(x1, y1)];
    return mix(mix(v00, v10, tx), mix(v01, v11, tx), ty);
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    if gid.x >= params.dye_width || gid.y >= params.dye_height { return; }

    let pos = vec2f(gid.xy);
    let i = dye_idx(gid.x, gid.y);

    let phys_x = u32(f32(gid.x) * f32(params.width) / f32(params.dye_width));
    let phys_y = u32(f32(gid.y) * f32(params.height) / f32(params.dye_height));
    if obstacles[vel_idx(phys_x, phys_y)] != 0u {
        dye_next[i] = vec4f();
        return;
    }

    let vel = sample_velocity(pos) * vec2f(f32(params.dye_width), f32(params.dye_height))
                                   / vec2f(f32(params.width), f32(params.height));
    let src_pos = pos - vel * params.dt;
    dye_next[i] = sample_dye(src_pos) / (1.0 + params.dye_dissipation * params.dt);
}

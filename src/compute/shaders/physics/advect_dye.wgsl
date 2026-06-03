struct Params {
    width: u32,
    height: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<uniform> dt: f32;
@group(0) @binding(2) var<storage, read> velocity: array<vec2f>;
@group(0) @binding(3) var<storage, read> dye: array<f32>;
@group(0) @binding(4) var<storage, read_write> dye_next: array<f32>;
@group(0) @binding(5) var<storage, read> obstacles: array<u32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

fn sample_dye(pos: vec2f) -> f32 {
    let x = clamp(pos.x, 0.0, f32(params.width - 1));
    let y = clamp(pos.y, 0.0, f32(params.height - 1));

    let x0 = u32(x);
    let y0 = u32(y);
    let x1 = min(x0 + 1, params.width - 1);
    let y1 = min(y0 + 1, params.height - 1);

    let tx = x - f32(x0);
    let ty = y - f32(y0);

    let v00 = dye[idx(x0, y0)];
    let v10 = dye[idx(x1, y0)];
    let v01 = dye[idx(x0, y1)];
    let v11 = dye[idx(x1, y1)];

    return mix(mix(v00, v10, tx), mix(v01, v11, tx), ty);
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    if gid.x >= params.width || gid.y >= params.height { return; }

    let pos = vec2f(gid.xy);
    let i = idx(gid.x, gid.y);
    let vel = velocity[i];
    let src_pos = pos - vel * dt;

    if obstacles[i] != 0u {
        dye_next[i] = 0.0;
        return;
    }

    const DISSIPATION = 0.2;
    dye_next[i] = sample_dye(src_pos) * (1.0 - DISSIPATION * dt);
}

struct Params { width: u32, height: u32 }

@group(0) @binding(0) var<uniform>             params:   Params;
@group(0) @binding(1) var<uniform>             dt:       f32;
@group(0) @binding(2) var<storage, read>       velocity: array<vec2f>;
@group(0) @binding(3) var<storage, read>       dye:      array<f32>;
@group(0) @binding(4) var<storage, read_write> dye_next: array<f32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

fn sample_dye(pos: vec2f) -> f32 {
    let max_bounds = vec2f(f32(params.width - 1u), f32(params.height - 1u));
    
    let clamped_pos = clamp(pos, vec2f(0.0), max_bounds);

    let xy0_f = floor(clamped_pos);

    let t = clamped_pos - xy0_f; 

    let xy0 = vec2u(xy0_f);
    let xy1 = min(xy0 + vec2u(1u), vec2u(params.width - 1u, params.height - 1u));

    let d00 = dye[idx(xy0.x, xy0.y)];
    let d10 = dye[idx(xy1.x, xy0.y)];
    let d01 = dye[idx(xy0.x, xy1.y)];
    let d11 = dye[idx(xy1.x, xy1.y)];

    return mix(mix(d00, d10, t.x), mix(d01, d11, t.x), t.y);
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    if (gid.x >= params.width || gid.y >= params.height) { return; }

    let pos     = vec2f(gid.xy);
    let i       = idx(gid.x, gid.y);
    let vel     = velocity[i];
    let src_pos = pos - vel * dt;

    dye_next[i] = sample_dye(src_pos);
}

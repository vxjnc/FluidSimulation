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
@group(0) @binding(1) var<storage, read> pressure: array<f32>;
@group(0) @binding(2) var<storage, read> divergence: array<f32>;
@group(0) @binding(3) var<storage, read_write> pressure_next: array<f32>;
@group(0) @binding(4) var<storage, read> obstacles: array<u32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.width || y >= params.height { return; }

    let i = idx(x, y);
    if obstacles[i] != 0u {
        pressure_next[i] = 0.0;
        return;
    }

    let x0 = select(x - 1, x, x == 0);
    let x1 = select(x + 1, x, x == params.width - 1);
    let y0 = select(y - 1, y, y == 0);
    let y1 = select(y + 1, y, y == params.height - 1);

    let p_left = select(pressure[idx(x0, y)], pressure[i], obstacles[idx(x0, y)] != 0u);
    let p_right = select(pressure[idx(x1, y)], pressure[i], obstacles[idx(x1, y)] != 0u);
    let p_down = select(pressure[idx(x, y0)], pressure[i], obstacles[idx(x, y0)] != 0u);
    let p_up = select(pressure[idx(x, y1)], pressure[i], obstacles[idx(x, y1)] != 0u);

    pressure_next[i] = (p_left + p_right + p_down + p_up - divergence[i]) * 0.25;
}

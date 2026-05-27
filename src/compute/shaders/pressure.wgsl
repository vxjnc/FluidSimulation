struct Params { width: u32, height: u32 }

@group(0) @binding(0) var<uniform>              params:         Params;
@group(0) @binding(1) var<storage, read>        pressure:       array<f32>;
@group(0) @binding(2) var<storage, read>        divergence:     array<f32>;
@group(0) @binding(3) var<storage, read_write>  pressure_next:  array<f32>;

fn idx(x: u32, y: u32) -> u32 {
    return y * params.width + x;
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if (x >= params.width || y >= params.height) { return; }

    let x0 = select(x - 1, x, x == 0);
    let x1 = select(x + 1, x, x == params.width  - 1);
    let y0 = select(y - 1, y, y == 0);
    let y1 = select(y + 1, y, y == params.height - 1);

    let p_left  = pressure[idx(x0, y)];
    let p_right = pressure[idx(x1, y)];
    let p_down  = pressure[idx(x, y0)];
    let p_up    = pressure[idx(x, y1)];

    pressure_next[idx(x, y)] = (p_left + p_right + p_down + p_up - divergence[idx(x, y)]) * 0.25;
}

struct Params {
    cx: u32,
    cy: u32,
    radius: u32,
    val: u32,
    width: u32,
    height: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read_write> obstacles: array<u32>;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.width || y >= params.height { return; }

    let dx = i32(x) - i32(params.cx);
    let dy = i32(y) - i32(params.cy);
    if dx * dx + dy * dy > i32(params.radius * params.radius) { return; }

    obstacles[y * params.width + x] = params.val;
}

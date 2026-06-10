struct Params {
    cx: f32,
    cy: f32,
    radius: f32,
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

    let cx = u32(params.cx * f32(params.width));
    let cy = u32(params.cy * f32(params.height));

    let radius = u32(params.radius * f32(params.height));
    let dx = i32(x) - i32(cx);
    let dy = i32(y) - i32(cy);
    if dx * dx + dy * dy > i32(radius * radius) { return; }

    obstacles[y * params.width + x] = params.val;
}

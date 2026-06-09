import package::common::Params;

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> src: array<u32>;
@group(0) @binding(2) var<storage, read_write> dst: array<u32>;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.dst_width || y >= params.dst_height { return; }

    let uv = (vec2f(f32(x), f32(y)) + 0.5) / vec2f(f32(params.dst_width), f32(params.dst_height));
    let pos = uv * vec2f(f32(params.src_width), f32(params.src_height)) - 0.5;
    let xy = vec2u(clamp(pos + 0.5, vec2f(0.0), vec2f(f32(params.src_width - 1u), f32(params.src_height - 1u))));

    dst[y * params.dst_width + x] = src[xy.y * params.src_width + xy.x];
}

import package::common::Params;

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> src: array<vec4f>;
@group(0) @binding(2) var<storage, read_write> dst: array<vec4f>;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if x >= params.dst_width || y >= params.dst_height { return; }

    let uv = (vec2f(f32(x), f32(y)) + 0.5) / vec2f(f32(params.dst_width), f32(params.dst_height));
    let pos = uv * vec2f(f32(params.src_width), f32(params.src_height)) - 0.5;
    let pos_clamped = clamp(pos, vec2f(0.0), vec2f(f32(params.src_width - 1u), f32(params.src_height - 1u)));

    let xy0 = vec2u(pos_clamped);
    let xy1 = min(xy0 + vec2u(1u), vec2u(params.src_width - 1u, params.src_height - 1u));
    let t = fract(pos_clamped);

    let d00 = src[xy0.y * params.src_width + xy0.x];
    let d10 = src[xy0.y * params.src_width + xy1.x];
    let d01 = src[xy1.y * params.src_width + xy0.x];
    let d11 = src[xy1.y * params.src_width + xy1.x];

    dst[y * params.dst_width + x] = mix(mix(d00, d10, t.x), mix(d01, d11, t.x), t.y);
}

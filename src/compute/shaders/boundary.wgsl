struct Params { width: u32, height: u32 }

@group(0) @binding(0) var<uniform>             params:   Params;
@group(0) @binding(1) var<storage, read_write> velocity: array<vec2f>;

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) gid: vec3u) {
    let x = gid.x;
    let y = gid.y;
    if (x >= params.width || y >= params.height) { return; }

    let on_left   = x == 0u;
    let on_right  = x == params.width  - 1u;
    let on_bottom = y == 0u;
    let on_top    = y == params.height - 1u;

    if (!on_left && !on_right && !on_bottom && !on_top) { return; }

    let i = y * params.width + x;
    var v = velocity[i];
    if (on_left  || on_right)  { v.x = 0.0; }
    if (on_top   || on_bottom) { v.y = 0.0; }
    velocity[i] = v;
}

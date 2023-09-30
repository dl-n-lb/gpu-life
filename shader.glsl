@vs vs
in vec4 vertex_pos;

void main() {
    gl_Position = vertex_pos;
}
@end

@fs fs_draw
uniform draw_params {
    vec2 resolution;
    vec2 scale_factor;
};

uniform sampler2D life;

out vec4 out_color;

void main() {
    out_color = texture(life, gl_FragCoord.xy / resolution);
}
@end

@fs fs_life
uniform life_params {
    vec2 resolution;
    vec2 scale_factor;
    vec2 m_pos;
    int add;
    int run;
};

uniform sampler2D life;

out vec4 out_color;

void main() {
    vec2 uv   = (gl_FragCoord.xy + vec2(0,  0)) / resolution * scale_factor;
    vec2 uvl  = (gl_FragCoord.xy + vec2(-1, 0)) / resolution * scale_factor;
    vec2 uvtl = (gl_FragCoord.xy + vec2(-1, 1)) / resolution * scale_factor;
    vec2 uvt  = (gl_FragCoord.xy + vec2(0,  1)) / resolution * scale_factor;
    vec2 uvtr = (gl_FragCoord.xy + vec2(1,  1)) / resolution * scale_factor;
    vec2 uvr  = (gl_FragCoord.xy + vec2(1,  0)) / resolution * scale_factor;
    vec2 uvbr = (gl_FragCoord.xy + vec2(1, -1)) / resolution * scale_factor;
    vec2 uvb  = (gl_FragCoord.xy + vec2(0, -1)) / resolution * scale_factor;
    vec2 uvbl = (gl_FragCoord.xy + vec2(-1, -1)) / resolution * scale_factor;

    vec4 c  = texture(life,   uv);
    vec4 l  = texture(life,  uvl);
    vec4 tl = texture(life, uvtl);
    vec4 t  = texture(life,  uvt);
    vec4 tr = texture(life, uvtr);
    vec4 r  = texture(life,  uvr);
    vec4 br = texture(life, uvbr);
    vec4 b  = texture(life,  uvb);
    vec4 bl = texture(life, uvbl);
    float nb_cnt = (l + tl + t + tr + r + br + b + bl).r;

    out_color = c;
    // TODO: replace ifs with maths
    if (run > 0) {
        if (c.r > 0) {
            if (nb_cnt < 2) {
                out_color = vec4(0);
            }
            else if (nb_cnt > 3) {
                out_color = vec4(0);
            } else {
                out_color = vec4(1);
            }
        } else if (nb_cnt == 3) {
            out_color = vec4(1);
        } else {
            out_color = vec4(0);
        }
    }

    if (add > 0 && length(m_pos / scale_factor - gl_FragCoord.xy) <= 0.5) {
        out_color = vec4(1);
    }
}
@end

@program life vs fs_life
@program draw vs fs_draw

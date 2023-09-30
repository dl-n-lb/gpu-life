#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

#include "badapple.c"

#include "shader.glsl.h"

// will reset on screen resize
// TODO: move ffmpeg to compilation;
// build a .h file with the encoded video stored in it
// and read from that into a *fixed size* buffer
// to send to the gpu :)

static int scr_w = 800;
static int scr_h = 600;
static bool run = false;

#define FACTOR 4
#define SLOW_FACTOR 2

static struct {
  int m_x, m_y;
  int pm_x, pm_y;
  bool clicked;
} input;

static struct {
  struct {
    sg_image target;
    sg_image dpth;
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass pass;
    sg_pass_action pass_action;
  } life[2]; // for odd and even frames, using the other as input
  struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
  } render; // draw to screen (dont need pass since it is default)
} state;

void setup_life_passes() {
  sg_image_desc img_dsc = {
      .render_target = true,
      .width = scr_w / FACTOR,
      .height = scr_h / FACTOR,
      .pixel_format = SG_PIXELFORMAT_RGBA32F,
      .min_filter = SG_FILTER_NEAREST,
      .mag_filter = SG_FILTER_NEAREST,
      .wrap_u = SG_WRAP_REPEAT,
      .wrap_v = SG_WRAP_REPEAT,
      .sample_count = 1,
  };
  sg_image_desc dpth_dsc = img_dsc;
  dpth_dsc.pixel_format = SG_PIXELFORMAT_DEPTH;
  for (int i = 0; i < 2; ++i) {
    state.life[i].target = sg_make_image(&img_dsc);
    state.life[i].dpth = sg_make_image(&dpth_dsc);
  }
  for (int i = 0; i < 2; ++i) {
    if (sg_query_pass_state(state.life[i].pass) == SG_RESOURCESTATE_VALID) {
      sg_destroy_pass(state.life[i].pass);
    }
    if (sg_query_buffer_state(state.life[i].bind.vertex_buffers[0]) ==
        SG_RESOURCESTATE_VALID) {
      sg_destroy_buffer(state.life[i].bind.vertex_buffers[0]);
    }
    if (sg_query_pipeline_state(state.life[i].pip) == SG_RESOURCESTATE_VALID) {
      sg_destroy_pipeline(state.life[i].pip);
    }
    state.life[i].pass_action =
        (sg_pass_action){.colors[0] = {.action = SG_ACTION_DONTCARE}};
    state.life[i].pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {.attrs[ATTR_vs_vertex_pos].format = SG_VERTEXFORMAT_FLOAT2},
        .shader = sg_make_shader(life_shader_desc(sg_query_backend())),
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA32F,
        .depth.pixel_format = SG_PIXELFORMAT_DEPTH,
    });

    float fsq_verts[] = {-1.f, -3.f, 3.f, 1.f, -1.f, 1.f};
    state.life[i].bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(fsq_verts),
    });
    state.life[i].bind.fs_images[SLOT_life] = state.life[i == 0].target;
    state.life[i].pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = state.life[i].target,
        .depth_stencil_attachment.image = state.life[i].dpth,
    });
  }
}

void init(void) {
  sg_setup(&(sg_desc){
      .context = sapp_sgcontext(),
      .logger.func = slog_func,
  });

  float fsq_verts[] = {-1.f, -3.f, 3.f, 1.f, -1.f, 1.f};
  state.render.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
      .data = SG_RANGE(fsq_verts),
      .label = "fsq vertices",
  });

  state.render.pip = sg_make_pipeline(&(sg_pipeline_desc){
      .layout =
          {
              .attrs[ATTR_vs_vertex_pos].format = SG_VERTEXFORMAT_FLOAT2,
          },
      .shader = sg_make_shader(draw_shader_desc(sg_query_backend())),
  });

  state.render.pass_action = (sg_pass_action){
      .colors[0] =
          {
              .action = SG_ACTION_CLEAR,
              .value = {0.18, 0.18, 0.18, 1},
          },
  };

  setup_life_passes();
}

void frame(void) {
  if (sapp_frame_count() % 100 == 0) {
    fprintf(stderr, "Current FPS: %f\n", 1.0 / sapp_frame_duration());
  }
  int w = sapp_width();
  int h = sapp_height();
  if (w != scr_w || h != scr_h) {
    scr_w = w;
    scr_h = h;
    fprintf(stderr, "resolution change: %d, %d\n", w, h);
    setup_life_passes();
  }

  int c = sapp_frame_count() % 2;
  state.render.bind.fs_images[SLOT_life] = state.life[c].target;
  sg_begin_pass(state.life[c].pass, &state.life[c].pass_action);
  {
    sg_apply_pipeline(state.life[c].pip);
    sg_apply_bindings(&state.life[c].bind);
    life_params_t fp = {
        .resolution = {scr_w, scr_h},
        .scale_factor = {FACTOR, FACTOR},
        .m_pos = {input.m_x, scr_h - input.m_y},
        .add = input.clicked,
        .run = run ? sapp_frame_count() % SLOW_FACTOR == 0 : 0,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_life_params, &SG_RANGE(fp));
    sg_draw(0, 3, 1);
  }
  sg_end_pass();

  sg_begin_default_pass(&state.render.pass_action, w, h);
  {
    sg_apply_pipeline(state.render.pip);
    sg_apply_bindings(&state.render.bind);
    draw_params_t dp = {.resolution = {scr_w, scr_h}};
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_draw_params, &SG_RANGE(dp));
    sg_draw(0, 3, 1);
  }
  sg_end_pass();
  sg_commit();
}

void event(const sapp_event *event) {
  switch (event->type) {
  case SAPP_EVENTTYPE_MOUSE_MOVE: {
    input.pm_x = input.m_x;
    input.pm_y = input.m_y;
    input.m_x = (int)event->mouse_x;
    input.m_y = (int)event->mouse_y;
    break;
  }
  case SAPP_EVENTTYPE_MOUSE_DOWN: {
    input.clicked = true;
    break;
  }
  case SAPP_EVENTTYPE_KEY_DOWN: {
    // run = !run;
    //  TODO: why doesnt this work?
    if (event->key_code == SAPP_KEYCODE_R ||
        event->key_code == SAPP_KEYCODE_SPACE) {
      run = !run;
    }
    break;
  }
  default: {
    input.clicked = false;
    break;
  }
  }
}

void cleanup(void) {
  sg_shutdown();
}

sapp_desc sokol_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .event_cb = event,
      .width = scr_w,
      .height = scr_h,
      .window_title = "i <3 fluids",
      .icon.sokol_default = true,
      .logger.func = slog_func,
  };
}

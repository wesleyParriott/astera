#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

#include <astera/asset.h>
#include <astera/render.h>
#include <astera/input.h>

#include "dungeon.h"

#define SPRITE_COUNT 16

r_shader      shader, baked, particle, fbo_shader;
r_sprite      sprite;
r_sheet       sheet;
r_ctx*        render_ctx;
i_ctx*        input_ctx;
r_baked_sheet baked_sheet;
r_particles   particles;

map_t map;

r_framebuffer fbo, ui_fbo;
r_anim        anim;

// spotlight effect
vec2  point_pos, point_size;
float view_size;

vec2 player_pos;

static int valid_tile(int type) {
  switch (type) {
    case FLOOR:
    case WALL:
    case CORNER:
    case DOOR1:
    case DOOR2:
    case ROCK:
    case TREASURE:
      return 1;
    default:
      return 0;
  }
}

r_shader load_shader(const char* vs, const char* fs) {
  asset_t* vs_data = asset_get(vs);
  asset_t* fs_data = asset_get(fs);

  r_shader shader = r_shader_create(vs_data->data, fs_data->data);

  asset_free(vs_data);
  asset_free(fs_data);

  return shader;
}

enum { LEFT = 1 << 1, RIGHT = 1 << 2, UP = 1 << 3, DOWN = 1 << 4 } dir;

static int rnd(int min, int max) {
  int range = max - min;
  int value = rand() % range;
  return min + value;
}

void particle_spawn(r_particles* system, r_particle* particle) {
  float x = fmod(rand(), system->size[0]);
  float y = fmod(rand(), system->size[1]);

  particle->position[0] = x;
  particle->position[1] = y;
  particle->layer       = 10;

  int dir_x = 0, dir_y = 0;

  dir_x = rnd(-2, 2);
  dir_y = rnd(-2, 2);

  particle->direction[0] = dir_x;
  particle->direction[1] = dir_y;
}

int get_floor_tex(int index) {
  if (index < 8) {
    return FLOOR_TEX0;
  } else {
    switch (index) {
      case 8:
      case 9:
        return FLOOR_TEX1;
      case 10:
      case 11:
        return FLOOR_TEX2;
      case 12:
      case 13:
        return FLOOR_TEX3;
      default:
        return FLOOR_TEX0;
    }
  }
  switch (index) {
    case 0:
      return FLOOR_TEX0;
    case 1:
      return FLOOR_TEX1;
    case 2:
      return FLOOR_TEX2;
    case 3:
      return FLOOR_TEX3;
    default:
      return FLOOR_TEX0;
  };
}

void particle_animate(r_particles* system, r_particle* particle) {
  float life_span = system->particle_life - particle->life;
  float progress  = life_span / system->particle_life;

  particle->frame = life_span / (MS_TO_SEC / system->render.anim.rate);

  particle->velocity[0] =
      (sin(progress * 3.1459) * particle->direction[0]) * 0.0075f;
  particle->velocity[1] =
      (sin(progress * 3.1459) * particle->direction[1]) * 0.0075f;
}

int get_tex_id(map_t map, int tile, int x, int y) {
  if (tile == WALL || tile == DOOR1 || tile == DOOR2 || tile == CORNER) {
    int direction = get_direction(map, x, y);

    if (IS_DIR(direction, DIR_LEFT)) {
      return LEFT_WALL;
    } else if (IS_DIR(direction, DIR_RIGHT)) {
      return RIGHT_WALL;
    } else if (IS_DIR(direction, DIR_TOP)) {
      return TOP_WALL;
    } else if (IS_DIR(direction, DIR_BOTTOM)) {
      return BOTTOM_WALL;
    }
  } else {
    if (tile == FLOOR) {
      int random_tex = rand() % 16;
      return get_floor_tex(random_tex);
    }
  }
}

void load_map() {
  if (map.tile_count) {
    map_free(&map);
    r_baked_sheet_destroy(&baked_sheet);
  }

  map = (map_t){0};
  map = map_gen(time(NULL) + rand());
  // map_cleanup(&map);
  output_map(map);

  int tile_index = 0;

  // Center camera
  float min_x = FLT_MAX, max_x = FLT_MIN, min_y = FLT_MAX, max_y = FLT_MIN;

  r_baked_quad* quads =
      (r_baked_quad*)malloc(sizeof(r_baked_quad) * (map.width * map.height));
  memset(quads, 0, sizeof(r_baked_quad) * map.tile_count);

  for (int y = 0; y < map.height; ++y) {
    for (int x = 0; x < map.width; ++x) {
      int tile = tile_at(&map, x, y);

      if (valid_tile(tile)) {
        vec2 pos = {x * 16.f, y * 16.f};

        if (pos[0] > max_x) {
          max_x = pos[0];
        }

        if (pos[0] < min_x) {
          min_x = pos[0];
        }

        if (pos[1] > max_y) {
          max_y = pos[1];
        }

        if (pos[1] < min_y) {
          min_y = pos[1];
        }

        int tex = get_tex_id(map, tile, x, y);

        quads[tile_index] = (r_baked_quad){
            .x      = pos[0],
            .y      = pos[1],
            .width  = 16.f,
            .height = 16.f,
            .subtex = tex,
            .layer  = 1,
            .flip_x = 0,
            .flip_y = 0,
        };

        ++tile_index;
      } else {
        if (tile == PLAYER) {
          player_pos[0] = x * 16.f;
          player_pos[1] = y * 16.f;
        }
      }
    }
  }

  printf("min [%f %f] max [%f %f]\n", min_x, min_y, max_x, max_y);

  vec2 center_pos = {min_x + ((max_x - min_x) / 2.f),
                     min_y + ((max_y - min_y) / 2.f)};
  // if (player_pos[0] != 0.f && player_pos[1] != 0.f)
  // vec2_dup(center_pos, player_pos);

  r_camera_center_to(r_ctx_get_camera(render_ctx), center_pos);
  printf("%f %f\n", center_pos[0], center_pos[1]);

  printf("map size: %i x %i, %i vs %i\n", map.width, map.height, map.tile_count,
         tile_index);

  vec2 baked_sheet_pos = {0.f, 0.f};
  baked_sheet =
      r_baked_sheet_create(&sheet, quads, map.tile_count, baked_sheet_pos);

  free(quads);
}

void init_render(r_ctx* ctx) {
  view_size = 0.8f;
  shader =
      load_shader("resources/shaders/main.vert", "resources/shaders/main.frag");
  r_shader_cache(ctx, shader, "main");

  baked = load_shader("resources/shaders/basic.vert",
                      "resources/shaders/basic.frag");
  r_shader_cache(ctx, baked, "baked");

  /*  particle = load_shader("resources/shaders/particles.vert",
                           "resources/shaders/particles.frag");
    r_shader_cache(ctx, particle, "particle");*/

  fbo_shader = load_shader("resources/shaders/dungeon_post.vert",
                           "resources/shaders/dungeon_post.frag");

  fbo    = r_framebuffer_create(1280, 720, fbo_shader);
  ui_fbo = r_framebuffer_create(1280, 720, fbo_shader);

  asset_t* sheet_data = asset_get("resources/textures/Dungeon_Tileset.png");
  sheet = r_sheet_create_tiled(sheet_data->data, sheet_data->data_length, 16,
                               16, 0, 0);

  asset_free(sheet_data);

  uint32_t anim_frames[6] = {0, 1, 2, 3, 4, 5};
  anim                    = r_anim_create(&sheet, anim_frames, 6, 6);
  anim.loop               = 1;

  load_map();

  // 16x9 * 20
  vec2 camera_size = {320, 180};
  r_camera_set_size(r_ctx_get_camera(ctx), camera_size);
}

void input(float delta) {
  if (i_key_clicked(input_ctx, KEY_ESCAPE)) {
    r_window_request_close(render_ctx);
  }

  if (i_key_clicked(input_ctx, 'G')) {
    //
    printf("gen\n");
    load_map();
  }

  if (i_key_clicked(input_ctx, 'P')) {
    vec2 pos;
    r_camera_get_position(r_ctx_get_camera(render_ctx), pos);
    printf("position: %.2f %.2f\n", pos[0], pos[1]);
  }

  vec3 camera_move = {0.f, 0.f, 0.f};

  if (i_key_down(input_ctx, 'A')) {
    camera_move[0] -= 8.f;
  }
  if (i_key_down(input_ctx, 'D')) {
    camera_move[0] += 8.f;
  }

  if (i_key_down(input_ctx, 'W')) {
    camera_move[1] = -1.f * 8.f;
  }
  if (i_key_down(input_ctx, 'S')) {
    camera_move[1] = 1.f * 8.f;
  }

  // vec3_scale(camera_move, camera_move, delta * 0.1f);
  r_camera_move(r_ctx_get_camera(render_ctx), camera_move);
}

void update(float delta) { // r_particles_update(&particles, delta); }
}

int main(void) {
  r_window_params params = r_window_params_create(1280, 720, 0, 0, 1, 0, 0,
                                                  "Dungeon Crawler Example");

  vec2 screen_size = {params.width, params.height};

  render_ctx = r_ctx_create(params, 0, 3, 128, 128, 4);
  r_window_clear_color("#0A0A0A");

  if (!render_ctx) {
    printf("Render context failed.\n");
    return 1;
  }

  input_ctx = i_ctx_create(16, 16, 0, 5, 32);

  if (!input_ctx) {
    printf("Input context failed.\n");
    return 1;
  }

  init_render(render_ctx);

  asset_t* icon = asset_get("resources/textures/icon.png");
  r_window_set_icon(render_ctx, icon->data, icon->data_length);
  asset_free(icon);

  r_ctx_make_current(render_ctx);
  r_ctx_set_i_ctx(render_ctx, input_ctx);

  while (!r_window_should_close(render_ctx)) {
    i_ctx_update(input_ctx);

    input(16.f);

    update(16.f);

    if (r_can_render(render_ctx)) {
      r_framebuffer_bind(fbo);
      r_window_clear();

      // r_particles_draw(render_ctx, &particles, particle);

      r_ctx_update(render_ctx);

      r_baked_sheet_draw(render_ctx, baked, &baked_sheet);

      r_ctx_draw(render_ctx);

      glViewport(0, 0, 1280, 720);
      r_set_v2(fbo.shader, "screen_size", screen_size);
      r_set_uniformf(fbo.shader, "view_size", view_size);
      r_framebuffer_draw(render_ctx, fbo);
      r_window_swap_buffers(render_ctx);
    }
  }

  r_ctx_destroy(render_ctx);
  i_ctx_destroy(input_ctx);

  free(render_ctx);
  free(input_ctx);

  return 0;
}

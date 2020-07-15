#include "dungeon.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int evened(int value) { return round(value / 2) * 2; }

static int rnd_range(int min, int max) { return (rand() % (max - min)) + min; }

static char get_char(int type) {
  switch (type) {
    case TREASURE:
      return '$';
    case ROCK:
      return 'R';
    case CORNER:
      return '!';
    case WALL:
      return '#';
    case FLOOR:
      return '.';
    case DOOR1:
      return '+';
    case DOOR2:
      return '\'';
    case PLAYER:
      return '@';
    case EMPTY:
      return ' ';
  }

  return type;
}

int tile_at(map_t* map, int x, int y) {
  return map->tiles[(y * map->width) + x];
}

void set_tile(map_t* map, int x, int y, int value) {
  map->tiles[(y * map->width) + x] = value;
}

room_t* add_room(map_t* map, room_t room) {
  map->rooms = realloc(map->rooms, sizeof(room_t) * (map->room_count + 1));
  map->rooms[map->room_count] = room;
  ++map->room_count;
}

void add_door(room_t* room, door_t door) {
  room->doors = realloc(room->doors, sizeof(door_t) * (room->door_count + 1));
  room->doors[room->door_count] = door;
  ++room->door_count;
}

void gen_room(map_t* map, int start) {
  int width  = (rand() % 10) + 5;
  int height = (rand() % 6) + 3;
  int left   = (rand() % (map->width - width - 2)) + 1;
  int top    = (rand() % (map->height - height - 2)) + 1;

  for (int y = top - 1; y < top + height + 2; y++)
    for (int x = left - 1; x < left + width + 2; x++)
      if (tile_at(map, x, y) == FLOOR)
        return;

  int doors = 0;
  int door_x, door_y;

  room_t tmp_room = (room_t){
      .x = left, .y = top, .width = width, .height = height, .door_count = 0};

  if (!start) {
    for (int y = top - 1; y < top + height + 2; y++)
      for (int x = left - 1; x < left + width + 2; x++) {
        int s = x < left || x > left + width;
        int t = y < top || y > top + height;
        if (s ^ t && tile_at(map, x, y) == WALL) {
          doors++;
          if (rand() % doors == 0)
            door_x = x, door_y = y;

          door_t door = (door_t){x, y, DOOR1};
          add_door(&tmp_room, door);
        }
      }

    if (doors == 0)
      return;
  }

  for (int y = top - 1; y < top + height + 2; y++)
    for (int x = left - 1; x < left + width + 2; x++) {
      int s = x < left || x > left + width;
      int t = y < top || y > top + height;
      set_tile(map, x, y, s && t ? CORNER : s ^ t ? WALL : FLOOR);
    }

  if (doors > 0) {
    set_tile(map, door_x, door_y, (rand() % 2) ? DOOR2 : DOOR1);
  }

  for (int j = 0; j < (start ? 1 : (rand() % 6) + 1); j++) {
    int x = (rand() % width) + left;
    int y = (rand() % height) + top;

    set_tile(map, x, y, (start ? PLAYER : rand() % 8) == 0 ? TREASURE : 0);
  }

  add_room(map, tmp_room);
}

int get_direction(map_t map, int x, int y) {
  for (int i = 0; i < map.room_count; ++i) {
    int left = 0, right = 0, top = 0, bottom = 0;

    room_t* room = &map.rooms[i];
    if (x == room->x) {
      left = 1;
    } else if (x == room->x + room->width) {
      right = 1;
    }

    if (y == room->y) {
      bottom = 1;
    } else if (y == room->y + room->height) {
      top = 1;
    }

    return (left) ? DIR_LEFT
                  : 0 | (right)
                        ? DIR_RIGHT
                        : 0 | (top) ? DIR_TOP : 0 | (bottom) ? DIR_BOTTOM : 0;
  }
}

map_t map_create(int width, int height) {
  map_t map =
      (map_t){.width = width, .height = height, .rooms = 0, .room_count = 0};
  map.tiles      = (int*)malloc(sizeof(int) * width * height);
  map.tile_count = width * height;
  map.room_count = 0;
  map.rooms      = 0;
  memset(map.tiles, 0, sizeof(int) * width * height);

  return map;
}

void map_free(map_t* map) {
  free(map->tiles);

  for (int i = 0; i < map->room_count; ++i) {
    free(map->rooms[i].doors);
  }

  map->tile_count = 0;
  map->room_count = 0;

  free(map->rooms);
}

void output_map(map_t map) {
  printf("map size: [%i x %i]\n", map.width, map.height);
  for (int y = 0; y < map.height; ++y) {
    for (int x = 0; x < map.width; ++x) {
      int  tile = tile_at(&map, x, y);
      char c    = get_char(tile);
      putchar(c == CORNER ? WALL : c);
      if (x == map.width - 1) {
        printf("\n");
      }
    }
  }
}

void map_cleanup(map_t* map) {
  // TODO redo, snake along axes
  if (map->room_count == 0) {
    printf("room count is 0\n");
    return;
  }

  if (!map->tiles) {
    printf("invalid tiles.\n");
    return;
  }

  int min_x = map->width, min_y = map->height, max_x = 0, max_y = 0;

  for (int x = 0; x < map->width; ++x) {
    for (int y = 0; y < map->height; ++y) {
      int tile = map->tiles[x + y * map->width];
      if (tile == WALL || tile == CORNER) {
        if (x < min_x)
          min_x = x;
        if (y < min_y)
          min_y = y;
        if (x > max_x)
          max_x = x;
        if (y > max_y)
          max_y = y;
      }
    }
  }

  if (min_x > 0)
    min_x -= 2;
  if (min_y > 0)
    min_y -= 2;
  if (max_x < map->width)
    max_x += 2;
  if (max_y < map->height)
    max_y += 2;

  int width  = (max_x - min_x);
  int height = (max_y - min_y);

  int* new_tiles = (int*)malloc(sizeof(int) * width * height);
  memset(new_tiles, EMPTY, sizeof(int) * width * height);

  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      new_tiles[x + width * y] =
          map->tiles[(min_x + x) + map->width * (min_y + y)];
    }
  }

  for (int i = 0; i < map->room_count; ++i) {
    map->rooms[i].x -= min_x;
    map->rooms[i].y -= min_y;
  }

  free(map->tiles);

  map->tiles      = new_tiles;
  map->tile_count = width * height;
  map->width      = width;
  map->height     = height;
}

map_t map_gen(long seed) {
  srand(seed);

  map_t map = map_create(80, 120);
  for (int j = 0; j < GEN_TRIES; ++j) {
    gen_room(&map, j == 0);
  }

  return map;
}


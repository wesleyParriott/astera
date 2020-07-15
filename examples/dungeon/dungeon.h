#ifndef DUNGEON_H
#define DUNGEON_H

#define MIN_DUNGEON_WIDTH  80
#define MAX_DUNGEON_WIDTH  120
#define MIN_DUNGEON_HEIGHT 80
#define MAX_DUNGEON_HEIGHT 140

#define MIN_WIDTH  5
#define MAX_WIDTH  15
#define MIN_HEIGHT 3
#define MAX_HEIGHT 9
#define GEN_TRIES  1000 // the number of attempts to make rooms

// Tex IDs in the tex sheet
#define TOP_LEFT_CORNER     0
#define TOP_RIGHT_CORNER    5
#define BOTTOM_LEFT_CORNER  49
#define BOTTOM_RIGHT_CORNER 54
#define TOP_WALL            1
#define BOTTOM_WALL         41
#define LEFT_WALL           10
#define RIGHT_WALL          14
#define LEFT_DOOR           47
#define RIGHT_DOOR          48
#define TOP_DOOR            67
#define BOTTOM_DOOR         68

#define FLOOR_TEX0 79
#define FLOOR_TEX1 19
#define FLOOR_TEX2 23
#define FLOOR_TEX3 24

#define IS_DIR(value, type) (((value & (type)) == type))

typedef enum {
  EMPTY    = 0,
  TREASURE = 1,
  ROCK     = 2,
  CORNER   = 3,
  WALL     = 4,
  FLOOR    = 5,
  DOOR1    = 6,
  DOOR2    = 7,
  PLAYER   = 8
} TILE_TYPE_T;

typedef enum {
  DIR_NONE   = 1 << 0,
  DIR_TOP    = 1 << 1,
  DIR_BOTTOM = 1 << 2,
  DIR_LEFT   = 1 << 3,
  DIR_RIGHT  = 1 << 4,
} DIRECTION_TYPE_T;

typedef struct {
  int x, y, type;
} door_t;

typedef struct {
  int     x, y, width, height;
  door_t* doors;
  int     door_count;
} room_t;

typedef struct {
  room_t* rooms;
  int     room_count;

  int* tiles;
  int  tile_count;
  int  width, height;
} map_t;

/* Create a map from seed */
map_t map_gen(long seed);

map_t map_create(int width, int height);
void  map_free(map_t* map);

void map_cleanup(map_t* map);

void gen_room(map_t* map, int start);

int get_direction(map_t map, int x, int y);

int     tile_at(map_t* map, int x, int y);
void    set_tile(map_t* map, int x, int y, int value);
room_t* add_room(map_t* map, room_t room);
void    add_door(room_t* room, door_t door);

void output_map(map_t map);

#endif

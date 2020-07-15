// Use this for all your pak needs
// usage: pakutil [add|remove|check|list|data] pak_file_path file ... file n

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <astera/asset.h>

typedef enum {
  NONE   = 0,
  ADD    = 1,
  REMOVE = 2,
  CHECK  = 3,
  LIST   = 4,
  DATA   = 5,
} usage_modes;

int main(int argc, char** argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "h") == 0 || strcmp(argv[1], "help") == 0 ||
        strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0 ||
        strcmp(argv[1], "--h") == 0 || strcmp(argv[1], "--help") == 0) {
      printf("Usage: ./pakutil [add|remove|check|list|data] dst.pak file ... "
             "file n\n");
      return 0;
    }
  }

  if (argc < 3) {
    printf("Invalid number of arguments passed to packer: %i\n", argc);
    return 1;
  }

  int mode = NONE;

  if (strcmp(argv[1], "add") == 0 || strcmp(argv[0], "a") == 0) {
    mode = ADD;
  } else if (strcmp(argv[1], "remove") == 0 || strcmp(argv[0], "r") == 0) {
    mode = REMOVE;
  } else if (strcmp(argv[1], "check") == 0 || strcmp(argv[0], "c") == 0) {
    mode = CHECK;
  } else if (strcmp(argv[1], "list") == 0 || strcmp(argv[0], "l") == 0) {
    mode = LIST;
  } else if (strcmp(argv[1], "data") == 0 || strcmp(argv[0], "d") == 0) {
    mode = DATA;
  }

  if (mode == NONE) {
    printf("Invalid mode passed\n");
    return 0;
  }

  const char* pak_file = argv[2];
  pak_t*      pak      = 0;

  switch (mode) {
    case ADD: {
      pak = pak_open_file(pak_file);

      if (!pak) {
        return 1;
      }

      for (int i = 3; i < argc; ++i) {
        const char* fp = argv[i];
        if (!pak_add_file(pak, fp)) {
          printf("Failed to add %s into pak file.\n", fp);
          return 1;
        }

        int count = pak_count(pak);
        printf("Added %s at index: %i\n", fp, count);
      }

      pak_write(pak);
    } break;
    case REMOVE: {
      int  entry_count = 0, entry_cap = argc - 3;
      int* entries = (int*)malloc(sizeof(int) * entry_cap);

      pak = pak_open_file(pak_file);

      if (!pak) {
        return 1;
      }

      for (int i = 3; i < argc; ++i) {
        entries[entry_count] = pak_find(pak, argv[i]);
        ++entry_count;
      }

      pak_close(pak);
      pak = pak_open_file(pak_file);

      if (!pak) {
        printf("Unable to reopen pak file for removal\n");
        return 1;
      }

      for (int i = 0; i < entry_count; ++i) {
        if (entries[i] != -1) {
          if (!pak_remove(pak, entries[i])) {
            printf("Unable to remove %s\n", argv[3 + i]);
          } else {
            printf("Removed %s\n", argv[3 + i]);
          }
        }
      }

      free(entries);

    } break;
    case CHECK: {
      pak = pak_open_file(pak_file);

      if (!pak) {
        return 1;
      }

      int count = pak_count(pak);
      printf("pak count: %i\n", count);

      for (int i = 3; i < argc; ++i) {
        int32_t find = pak_find(pak, argv[i]);

        if (find > -1) {
          printf("Matched %s at index %i in pak file\n", argv[i], find);
        } else {
          printf("No match found for %s in pak file\n", argv[i]);
        }
      }

    } break;
    case LIST: {
      pak = pak_open_file(pak_file);

      if (!pak) {
        return 1;
      }

      int count = pak_count(pak);
      printf("pak contains: %i entries.\n", count);
      for (int i = 0; i < count; ++i) {
        printf("%i: %s\n", i, pak_name(pak, i));
      }
    } break;
    case DATA: {
      pak = pak_open_file(pak_file);

      if (!pak) {
        printf("Unable to open pak file.\n");
        return 1;
      }

      for (int i = 3; i < argc; ++i) {
        int find = pak_find(pak, argv[i]);
        if (find != -1) {
          unsigned char* data = pak_extract(pak, find, 0);
          printf("%s:\n%s\n", argv[i], data);
          free(data);
        }
      }

    } break;
    default:
      printf("How did you get here?");
      break;
  }

  pak_close(pak);

  return 0;
}

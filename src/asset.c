// TODO: Combine pak_t & asset_map_t
/* NOTE: The PAK System is inspired by the r-lyeh/sdarc.c implementation
 */
#include <astera/asset.h>
#include <astera/debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(alloca)
#define alloca(x) __builtin_alloca(x)
#endif

#include <sys/stat.h>

#include <xxhash/xxhash.h>

static inline uint32_t pak_swap32(uint32_t t) {
  return (t >> 24) | (t << 24) | ((t >> 8) & 0xff00) | ((t & 0xff00) << 8);
}

#if defined(_M_IX86) || defined(_M_X64) // #ifdef LITTLE
#define htob32(x) pak_swap32(x)
#define btoh32(x) pak_swap32(x)
#define htol32(x) (x)
#define ltoh32(x) (x)
#else
#define htob32(x) (x)
#define btoh32(x) (x)
#define htol32(x) pak_swap32(x)
#define ltoh32(x) pak_swap32(x)
#endif

uint64_t asset_hash(asset_t* asset) {
  XXH64_hash_t val = XXH64(asset->data, asset->data_length, 1222);
  return (uint64_t)val;
}

pak_t* pak_open_file(const char* file) {
  struct stat _filestats;
  int         exists = (stat(file, &_filestats) == 0);

  if (!exists) {
#if !defined(ASTERA_NO_PAK_WRITE)
    FILE* f = fopen(file, "wb+");
    fclose(f);
#endif
  }

  FILE* f = fopen(file, "rb+");

  if (!f) {
    ASTERA_FUNC_DBG("unable to open file %s\n", file);
    return 0;
  }

  pak_header_t header = {0};
  fread(&header, 1, sizeof(pak_header_t), f);

  header.count     = ltoh32(header.count);
  header.file_size = ltoh32(header.file_size);

  if (!memcmp(&header.id, "PACK", 4)) {
    ASTERA_FUNC_DBG("invalid pak file format %s\n", file);
    fclose(f);
    return 0;
  }

  uint8_t write_only = 0;
  if (header.count == 0) {
    ASTERA_FUNC_DBG("empty pak file %s\n", file);
    write_only = 1;
    fclose(f);
  }

  pak_t *pak = (pak_t*)malloc(sizeof(pak_t)), zero = {0};

  if (write_only) {
    pak->file_mode    = 1;
    pak->count        = 0;
    pak->entries      = 0;
    pak->file_size    = 0;
    pak->filepath     = file;
    pak->changes      = 0;
    pak->change_count = 0;
    pak->has_change   = 0;
    return pak;
  }

  if (!pak) {
    ASTERA_FUNC_DBG("unable to alloc pak_t\n");
    fclose(f);
    return 0;
  }

  memset(pak, 0, sizeof(pak_t));
  pak->file_mode = 1;
  pak->count     = header.count;
  pak->file_size = header.file_size;

#if !defined(ASTERA_NO_PAK_WRITE)
  pak->changes      = 0;
  pak->change_count = 0;
  pak->has_change   = 0;
#endif

  // Seek to the end of the header size (start of entries)
  fseek(f, sizeof(pak_header_t), SEEK_SET);

  pak->entries = (pak_file_t*)malloc(sizeof(pak_file_t) * header.count);

  if (!pak->entries) {
    ASTERA_FUNC_DBG("unable to allocate space for entries\n");
    free(pak);
    return 0;
  }

  if (fread(pak->entries, header.count, sizeof(pak_file_t), f) !=
      sizeof(pak_file_t)) {
    ASTERA_FUNC_DBG("invalid Read of entries: %s\n", file);
    fclose(f);
    free(pak->entries);
    free(pak);
    return 0;
  }

  for (uint32_t i = 0; i < header.count; ++i) {
    pak_file_t* f = &pak->entries[i];
    f->offset     = ltoh32(f->offset);
    f->size       = ltoh32(f->size);
  }

  fclose(f);

  return pak;
}

pak_t* pak_open_mem(unsigned char* data, uint32_t data_length) {
  if (!data || !data_length) {
    ASTERA_FUNC_DBG("no data passed\n");
    return 0;
  }

  pak_t* pak = (pak_t*)malloc(sizeof(pak_t));

  if (!pak) {
    ASTERA_FUNC_DBG("unable to allocate space for pak header\n");
    return 0;
  }

  memset(pak, 0, sizeof(pak_t));

  pak->file_mode = 0;
  pak->entries   = 0;
  pak->file_size = 0;

#if !defined(ASTERA_NO_PAK_WRITE)
  pak->changes      = 0;
  pak->change_count = 0;
  pak->has_change   = 0;
#endif

  // Get the header from the start of the data passed
  pak_header_t* header = (pak_header_t*)data;

  if (!header) {
    ASTERA_FUNC_DBG("unable to obtain header pointer\n");
    free(pak);
    return 0;
  }

  if (memcmp(header->id, "PACK", 4)) {
    ASTERA_FUNC_DBG("invalid pak file\n");
    free(pak);
    return 0;
  }

  header->count     = ltoh32(header->count);
  header->file_size = ltoh32(header->file_size);

  // basically just header->count * 64, but safest to use sizeof
  uint32_t file_offset = header->count * sizeof(pak_file_t);
  pak->entries         = (pak_file_t*)data + sizeof(pak_header_t);

  for (uint32_t i = 0; i < header->count; ++i) {
    pak->entries[i].size   = ltoh32(pak->entries[i].size);
    pak->entries[i].offset = ltoh32(pak->entries[i].offset);
  }

  return 1;
}

#if !defined(ASTERA_NO_PAK_WRITE)
uint8_t pak_remove(pak_t* pak, uint32_t entry) {
  if (entry > -1) {
    ASTERA_FUNC_DBG("Invalid entry index passed: %i\n", entry);
  }

  if (!pak->file_mode) {
    ASTERA_FUNC_DBG("pak file must be opened in file mode to remove entries\n");
    return 0;
  }

  if (!pak) {
    ASTERA_FUNC_DBG("No pak structure passed.\n");
    return 0;
  }

  if (entry > pak->count) {
    ASTERA_FUNC_DBG("index passed greater than count.\n");
    return 0;
  }

  pak_change_t change = (pak_change_t){0};
  change.file         = &pak->entries[entry];
  change.type         = PAK_REMOVE;

  ++pak->change_count;
  pak->changes =
      (pak_change_t*)realloc(pak->changes, sizeof(pak->change_count));

  if (!pak->changes) {
    ASTERA_FUNC_DBG("unable to realloc pak changes array\n");
    return 0;
  }

  pak->changes[pak->change_count - 1] = change;
  pak->has_change                     = 1;

  return 1;
}

uint8_t pak_add_file(pak_t* pak, const char* filename) {
  if (!pak) {
    ASTERA_FUNC_DBG("no pak struct passed.\n");
    return 0;
  }

  if (!pak->file_mode) {
    ASTERA_FUNC_DBG("pak file must be opened in file mode to add entries\n");
    return 0;
  }

  if (!filename) {
    ASTERA_FUNC_DBG("no filename passed.\n");
    return 0;
  }

  uint32_t change = pak->change_count;
  ++pak->change_count;

  pak->changes = (pak_change_t*)realloc(pak->changes, pak->change_count *
                                                          sizeof(pak_change_t));

  if (!pak->changes) {
    ASTERA_FUNC_DBG("unable to relloc changes\n");
    return 0;
  }

  pak->changes[change] = (pak_change_t){
      .file          = 0,
      .filename      = filename,
      .type          = PAK_ADD_FILE,
      .data.filepath = filename,
  };

  // pak->changes[change].data.filepath = filename;

  return 1;
}

uint8_t pak_add_mem(pak_t* pak, const char* filename, unsigned char* data,
                    uint32_t data_length) {
  if (!pak) {
    ASTERA_FUNC_DBG("no pak header passed\n");
    return 0;
  }

  if (!pak->file_mode) {
    ASTERA_FUNC_DBG("pak file must be opened in file mode to add entries\n");
    return 0;
  }

  if (!filename) {
    ASTERA_FUNC_DBG("no filename passed\n");
    return 0;
  }

  if (!data || !data_length) {
    ASTERA_FUNC_DBG("no data passed\n");
  }

  asset_t asset     = (asset_t){0};
  asset.data        = data;
  asset.data_length = data_length;

  pak_change_t change = (pak_change_t){
      .filename   = filename,
      .type       = PAK_ADD_MEM,
      .file       = 0,
      .data.asset = asset,
  };

  // change.data.asset = asset;

  ++pak->change_count;
  pak->changes = (pak_change_t*)realloc(pak->changes, (pak->change_count) *
                                                          sizeof(pak_change_t));

  if (!pak->changes) {
    ASTERA_FUNC_DBG("unable to realloc pak changes array\n");
    return 0;
  }

  pak->changes[pak->change_count - 1] = change;
  return 1;
}
#endif

unsigned char* pak_extract(pak_t* pak, uint32_t index, uint32_t* size) {
  if (!pak) {
    ASTERA_FUNC_DBG("no pak header passed\n");
    return 0;
  }

  if (index > pak->count) {
    ASTERA_FUNC_DBG("entry outside of pak structure entry count\n");
    return 0;
  }

  pak_file_t* entry = &pak->entries[index];

  if (pak->file_mode) {
    FILE* f = fopen(pak->filepath, "rb+");

    if (!f) {
      ASTERA_FUNC_DBG("unable to open pak file %s\n", pak->filepath);
      return 0;
    }

    unsigned char* data =
        (unsigned char*)malloc(sizeof(unsigned char) * entry->size);

    if (!data) {
      ASTERA_FUNC_DBG("unable to allocate %i bytes\n",
                      (sizeof(unsigned char) * entry->size));
      fclose(f);
      return 0;
    }

    memset(data, 0, sizeof(unsigned char) * entry->size);

    fseek(f, entry->offset, SEEK_SET);
    if (!fread(data, sizeof(unsigned char), entry->size, f)) {
      ASTERA_FUNC_DBG("unable to read %i bytes\n",
                      (sizeof(unsigned char) * entry->size));
      fclose(f);
      free(data);
      return 0;
    }

    if (size) {
      *size = entry->size;
    }

    return data;
  } else {
    unsigned char* data = (unsigned char*)(pak->data_ptr + entry->offset);

    if (size) {
      *size = entry->size;
    }

    return data;
  }

  return 0;
}

uint32_t pak_extract_noalloc(pak_t* pak, uint32_t index, unsigned char* out,
                             uint32_t out_cap) {
  if (!out || !out_cap) {
    ASTERA_FUNC_DBG("invalid out buffer parameters passed\n");
    return 0;
  }

  if (index > pak->count) {
    ASTERA_FUNC_DBG("index outside of pak entry count\n");
    return 0;
  }

  pak_file_t* entry = &pak->entries[index];

  if (pak->file_mode) {
    FILE* f = fopen(pak->filepath, "rb+");

    if (!f) {
      ASTERA_FUNC_DBG("unable to open pak file %s\n", pak->filepath);
      return 0;
    }

    fseek(f, entry->offset, SEEK_SET);
    if (!fread(out, sizeof(unsigned char), entry->size, f)) {
      ASTERA_FUNC_DBG("unable to read %i bytes\n",
                      (sizeof(unsigned char) * entry->size));
      fclose(f);
      free(out);
      return 0;
    }

    return entry->size;
  } else {
    unsigned char* data = (unsigned char*)(pak->data_ptr + entry->offset);
    memcpy(out, data, sizeof(unsigned char) * entry->size);
    return entry->size;
  }

  return 0;
}

static uint32_t fs_file_size(const char* fp) {
  if (!fp) {
    ASTERA_FUNC_DBG("no file path passed\n");
    return 0;
  }

  FILE* f = fopen(fp, "r");

  if (!f) {
    ASTERA_FUNC_DBG("unable to open file %s\n", fp);
    return 0;
  }

  fseek(f, 0, SEEK_END);
  uint32_t length = (uint32_t)ftell(f);
  fclose(f);

  return length;
}

#if !defined(ASTERA_NO_PAK_WRITE)
uint8_t pak_write(pak_t* pak) {
  if (pak->has_change && pak->filepath && pak->file_mode) {
    uint32_t entry_count = pak->count;

    for (uint32_t i = 0; i < pak->change_count; ++i) {
      pak_change_t* change = &pak->changes[i];
      if (change->type == PAK_REMOVE) {
        uint8_t valid = 0;

        // Double check all the names to get an accurate representation
        // of how many entries we'll actually be writing
        for (uint32_t j = 0; j < pak->count; ++j) {
          if (strcmp(pak->entries[i].name, change->file->name) == 0) {
            valid = 1;
            break;
          }
        }

        if (valid)
          --entry_count;
      } else if (change->type == PAK_ADD_FILE || change->type == PAK_ADD_MEM) {
        uint8_t valid = 0;

        for (uint32_t j = 0; j < pak->count; ++j) {
          if (strcmp(pak->entries[i].name, change->filename) == 0) {
            valid = 1;
            break;
          }
        }

        if (valid)
          ++entry_count;
      }
    }

    pak_write_t* writes =
        (pak_write_t*)malloc(sizeof(pak_write_t) * entry_count);
    uint32_t write_count = 0;

    if (!writes) {
      ASTERA_FUNC_DBG("unable to allocate space for write table\n");
      return 0;
    }

    memset(writes, 0, sizeof(pak_write_t) * entry_count);

    // all of these will be PAK_KEEP types
    for (uint32_t i = 0; i < pak->count; ++i) {
      pak_file_t* entry  = &pak->entries[i];
      uint8_t     remove = 0;

      uint32_t      entry_size = entry->size;
      pak_change_t* change     = 0;

      for (uint32_t j = 0; j < pak->change_count; ++j) {
        pak_change_t* change = &pak->changes[i];
        if (!strcmp(change->filename, entry->name)) {
          if (change->type == PAK_REMOVE) {
            remove = 1;
          } else if (change->type == PAK_MODIFY_MEM) {
            entry_size = change->data.asset.data_length;
            change     = change;
          } else if (change->type == PAK_MODIFY_FILE) {
            entry_size = fs_file_size(change->data.filepath);
            change     = change;
          }

          break;
        }
      }

      if (!remove) {
        pak_write_t* write = &writes[write_count];

        if (change) {
          if (change->type == PAK_ADD_MEM) {
            write->data.asset = change->data.asset;
          } else if (change->type == PAK_ADD_FILE) {
            write->data.filepath = change->data.filepath;
          }

          write->type = change->type;
        } else {
          write->data.file = entry;
          write->type      = PAK_KEEP;
        }

        memcpy(write->name, entry->name, sizeof(char) * 56);
        write->size   = entry_size;
        write->offset = 0;
        ++write_count;
      }
    }

    for (uint32_t i = 0; i < pak->change_count; ++i) {
      pak_change_t* change = &pak->changes[i];

      uint32_t entry_size = 0;
      int8_t   add        = 0;

      if (change->type == PAK_ADD_MEM) {
        entry_size = change->data.asset.data_length;
        add        = 1;
      } else if (change->type == PAK_ADD_FILE) {
        entry_size = fs_file_size(change->data.filepath);
        add        = 1;
      }

      if (entry_size == 0) {
        ASTERA_FUNC_DBG("unable to get entry size for addition %i\n", i);
        break;
      }

      if (add) {
        pak_write_t* write = &writes[write_count];

        uint32_t namelen = strlen(change->filename);
        memcpy(write->name, change->filename,
               sizeof(char) * (namelen > 55) ? 55 : namelen);
        memset(write->name + (namelen > 55) ? 55 : namelen, 0, 56 - namelen);

        write->size   = entry_size;
        write->offset = 0;
        ++write_count;
      }
    }

    // Calculate the new offsets
    uint32_t offset = 12 + (write_count * 64);
    for (uint32_t i = 0; i < write_count; ++i) {
      pak_write_t* write = &writes[i];
      write->offset      = offset;
      offset += write->size;
    }

    struct stat _filestats;
    int         exists = (stat("tmp.pak", &_filestats) == 0);

    if (exists) {
      // Remove the temporary pak file if it exists
      if (!remove("tmp.pak")) {
        ASTERA_FUNC_DBG("unable to remove temporary file tmp.pak, failing\n");
        return 0;
      }
    }

    // Temporary file to add info to then rename to pak file being replaced
    FILE* tmp = fopen("tmp.pak", "wb+");

    if (!tmp) {
      ASTERA_FUNC_DBG("unable to open tmp.pak for intermediate rewrite\n");
      return 0;
    }

    FILE* base = fopen(pak->filepath, "rb+");
    if (!base) {
      ASTERA_FUNC_DBG("unable to open %s for intermediate rewrite\n",
                      pak->filepath);
    }

    fwrite("PACK", 1, 4, tmp);

    uint32_t count = ltoh32(write_count);
    fwrite(&count, 1, 4, tmp);

    uint32_t file_size = ltoh32(offset);
    fwrite(&file_size, 1, 4, tmp);

    for (uint32_t i = 0; i < write_count; ++i) {
      pak_write_t* write = &writes[i];

      fwrite(write->name, 56, sizeof(char), tmp);

      uint32_t _offset = ltoh32(write->offset);
      uint32_t _size   = ltoh32(write->size);

      fwrite(&_offset, 1, 4, tmp);
      fwrite(&_size, 1, 4, tmp);
    }

    char rewrite_buff[128] = {0};
    for (uint32_t i = 0; i < write_count; ++i) {
      pak_write_t* write = &writes[i];

      if (write->type == PAK_ADD_MEM) {
        fwrite(write->data.asset.data, write->data.asset.data_length, 1, tmp);
      } else if (write->type == PAK_ADD_FILE) {
        FILE* read_file = fopen(write->data.filepath, "r+");
        if (!read_file) {
          ASTERA_FUNC_DBG("unable to open file for read %s\n",
                          write->data.filepath);
          return 0;
        }

        uint32_t remaining = write->size;
        uint32_t check     = 0;
        while (!feof(read_file) && !ferror(read_file)) {
          memset(rewrite_buff, 0, sizeof(char) * 128);

          uint32_t read_amount = (remaining > 128) ? 128 : remaining;
          uint32_t bytes_read =
              fread(read_file, read_amount, sizeof(char), rewrite_buff);

          check += bytes_read;
          if (bytes_read == 0)
            break;

          fwrite(rewrite_buff, bytes_read, sizeof(char), tmp);
        }

        if (check != write->size) {
          ASTERA_FUNC_DBG("invalid file size read, expected %i read %i\n",
                          write->size, check);
          return 0;
        }
      } else if (write->type == PAK_KEEP) {
        if (!base) {
          ASTERA_FUNC_DBG("unable to open base file, failing\n");
          return 0;
        }

        fseek(base, write->data.file->offset, SEEK_SET);

        uint32_t remaining = write->size;
        uint32_t check     = 0;
        while (!feof(base) && !ferror(base)) {
          memset(rewrite_buff, 0, sizeof(char) * 128);

          uint32_t read_amount = (remaining > 128) ? 128 : remaining;
          uint32_t bytes_read =
              fread(base, read_amount, sizeof(char), rewrite_buff);

          check += bytes_read;
          if (bytes_read == 0)
            break;

          fwrite(rewrite_buff, bytes_read, sizeof(char), tmp);
        }

        if (check != write->size) {
          ASTERA_FUNC_DBG("invalid file size read, expected %i read %i\n",
                          write->size, check);
          return 0;
        }
      }
    }

    fclose(base);
    fclose(tmp);

    free(pak->changes);
    pak->changes      = 0;
    pak->change_count = 0;
    pak->has_change   = 0;

    // remove old pakfile & replace it with new one
    // remove(pak->filepath);
    // rename("tmp.pak", pak->filepath);

    return 1;
  } else {
    ASTERA_FUNC_DBG("no changes in pak struct\n");
  }

  return 0;
}

uint8_t asset_map_write(asset_map_t* map) {
  if (map->pak) {
    for (uint32_t i = 0; i < map->count; ++i) {
      if (map->assets[i]) {
        asset_t* asset = map->assets[i];
        if (!asset->name)
          continue;

        int32_t id = pak_find(map->pak, asset->name);
        if (!id) {
          if (!pak_add_mem(map->pak, asset->name, asset->data,
                           asset->data_length)) {
            return 0;
          }
        }
      }
    }
    return pak_write(map->pak);
  }

  ASTERA_FUNC_DBG("no pak file to write to\n");
  return 0;
}
#endif

// TODO add 'checksum' to pak file
uint8_t pak_close(pak_t* pak) {
  if (!pak) {
    // Maybe I should remove this....
    ASTERA_FUNC_DBG("invalid pak header passed\n");
    return 0;
  }

  if (pak->has_change && pak->filepath && pak->file_mode) {
    pak_write(pak);
  }

  if (pak->data_ptr) {
    free(pak->data_ptr);
  }

  if (pak->entries) {
    free(pak->entries);
  }

#if !defined(ASTERA_NO_PAK_WRITE)
  if (pak->changes)
    free(pak->changes);
#endif

  // free out the pointer itself
  free(pak);

  return 1;
}

int32_t pak_find(pak_t* pak, const char* filename) {
  if (!pak)
    return -1;

  for (uint32_t i = 0; i < pak->count; ++i) {
    if (!strcmp(pak->entries[i].name, filename)) {
      return i;
    }
  }

  return -1;
}

// I've overcomplicated this function, lol
// I can simplify it in a pretty obvious way I didn't think about at first for
// some reason
// O
// I'm gonna make an intermediary type for this that'll transfer things
// quicker

uint32_t pak_count(pak_t* pak) {
  if (!pak)
    return 0;

  return pak->count;
}

uint32_t pak_offset(pak_t* pak, uint32_t index) {
  if (!pak)
    return 0;
  return index < pak->count ? pak->entries[index].offset : 0;
}

uint32_t pak_size(pak_t* pak, uint32_t index) {
  if (!pak)
    return 0;
  return index < pak->count ? pak->entries[index].size : 0;
}

char* pak_name(pak_t* pak, uint32_t index) {
  if (!pak)
    return 0;
  return (index < pak->count) ? pak->entries[index].name : 0;
}

void asset_free(asset_t* asset) {
  asset->name   = 0;
  asset->filled = 0;
  asset->req    = 0;
  free(asset->data);

  // these pointers are allocated independent of anything
  if (asset->fs)
    free(asset);
}

void asset_map_free(asset_map_t* map) {
  if (!map) {
    ASTERA_FUNC_DBG("No map passed to free.\n");
    return;
  }

  for (uint32_t i = 0; i < map->capacity; ++i) {
    asset_t* asset = map->assets[i];
    if (asset->data) {
      free(asset->data);
    }
  }

  free(map->assets);
  map->assets   = 0;
  map->count    = 0;
  map->capacity = 0;
  map->name     = 0;
  map->filename = 0;
}

asset_t* asset_map_get(asset_map_t* map, const char* file) {
  // First check if we have it already loaded
  if (map->count > 0) {
    for (uint32_t i = 0; i < map->capacity; ++i) {
      if (map->assets[i]) {
        if (map->assets[i]->name && map->assets[i]->data) {
          if (strcmp(file, map->assets[i]->name) == 0) {
            return map->assets[i];
          }
        }
      }
    }
  }

  if (map->pak) {
    asset_t* asset = (asset_t*)malloc(sizeof(asset_t));
    asset->name    = file;
    ++map->uid_counter;
    asset->uid      = map->uid_counter;
    asset->req      = 0;
    asset->req_free = 0;
    asset->fs       = 0;

    int32_t asset_index = pak_find(map->pak, file);

    if (asset_index == -1) {
      free(asset);
      return 0;
    }

    // asset->data_length = pak_size(map->pak, asset_index);
    asset->data = pak_extract(map->pak, asset_index, &asset->data_length);

    asset->filled = 1;
    return asset;
  } else {
    asset_t* asset = asset_get(file);
    ++map->uid_counter;
    asset->uid = map->uid_counter;

    if (asset) {
      int8_t isset = 0;
      for (uint32_t i = 0; i < map->capacity; ++i) {
        if (!map->assets[i]) {
          map->assets[i] = asset;
          ++map->count;
          return asset;
        }
      }
    }
    asset->filled = 1;
    return asset;
  }

  return 0;
}

// NOTE: Local System
asset_t* asset_get(const char* file) {
  if (!file) {
    ASTERA_FUNC_DBG("no file requested.\n");
    return 0;
  }

  asset_t* asset = (asset_t*)malloc(sizeof(asset_t));

  FILE* f = fopen(file, "r+b");

  if (!f) {
    ASTERA_FUNC_DBG("Unable to open system file: %s\n", file);
    return 0;
  }

  fseek(f, 0, SEEK_END);
  uint32_t file_size = ftell(f);
  rewind(f);

  unsigned char* data =
      (unsigned char*)malloc(sizeof(unsigned char) * (file_size + 1));

  if (!data) {
    ASTERA_FUNC_DBG("Unable to allocate %i bytes for file %s\n", file_size,
                    file);
    free(asset);
    fclose(f);
    return 0;
  }

  uint32_t data_read = fread(data, sizeof(unsigned char), file_size, f);

  if (data_read != file_size) {
    ASTERA_FUNC_DBG("Incomplete read: %i expeceted, %i read.\n", file_size,
                    data_read);
    free(data);
    free(asset);
    return 0;
  }

  // NULL-Terminate the data
  data[data_read] = 0;

  fclose(f);

  asset->data        = data;
  asset->data_length = data_read;

  asset->name     = file;
  asset->filled   = 1;
  asset->req      = 0;
  asset->req_free = 0;
  asset->chunk    = 0;
  asset->fs       = 1;

  return asset;
}

void asset_map_add(asset_map_t* map, asset_t* asset) {
  for (uint32_t i = 0; i < map->capacity; ++i) {
    if (!map->assets[i]) {
      map->assets[i] = asset;
      asset->uid     = i;
      break;
    }
  }
}

void asset_map_remove(asset_map_t* map, asset_t* asset) {
  asset_map_removei(map, asset->uid);
}

void asset_map_removei(asset_map_t* map, uint32_t id) {
  if (!map->assets[id]) {
    ASTERA_FUNC_DBG("no asset at index %i on asset map.\n", id);
    return;
  }

  free(map->assets[id]->data);
  map->assets[id]->uid = 0;
}

void asset_map_update(asset_map_t* map) {
  for (uint32_t i = 0; i < map->capacity; ++i) {
    if (map->assets[i]->filled && map->assets[i]->req_free) {
      asset_map_removei(map, i);
    }
  }
}

asset_t* asset_get_chunk(const char* file, uint32_t chunk_start,
                         uint32_t chunk_length) {
  if (!file) {
    ASTERA_FUNC_DBG("No file requested\n");
    return 0;
  }

  asset_t* asset = (asset_t*)malloc(sizeof(asset_t));
  FILE*    f     = fopen(file, "r+b");

  if (!f) {
    ASTERA_FUNC_DBG("Unable to open system file: %s\n", file);
    free(asset);
    return 0;
  }

  fseek(f, 0, SEEK_END);
  uint32_t file_size = ftell(f);
  rewind(f);

  if (chunk_start > file_size) {
    ASTERA_FUNC_DBG("Chunk requested starts out of bounds [%i] of the "
                    "file [%i].\n",
                    chunk_start, file_size);
    asset->req   = 0;
    asset->chunk = 0;
    return 0;
  }

  // Allow for size clamping, in case we just want the data and not to have
  // to analyze it prior
  uint32_t max_length = (chunk_start + chunk_length > file_size)
                            ? file_size - chunk_start
                            : chunk_length;

  unsigned char* data =
      (unsigned char*)malloc(sizeof(unsigned char) * (max_length + 1));

  if (!data) {
    ASTERA_FUNC_DBG("Unable to allocate [%i] bytes for file chunk %s\n",
                    max_length, file);
    free(asset);
    fclose(f);
    return 0;
  }

  uint32_t data_read = fread(data, sizeof(unsigned char), max_length, f);

  if (data_read != max_length) {
    ASTERA_FUNC_DBG("Incomplete read: %i expeceted, %i read.\n", max_length,
                    data_read);
    free(asset);
    free(data);
    return 0;
  }

  data[data_read] = 0;

  fclose(f);

  asset->data_length = data_read;
  asset->data        = data;

  asset->name        = file;
  asset->filled      = 1;
  asset->chunk       = 1;
  asset->chunk_start = chunk_start;
  asset->req         = 0;
  asset->req_free    = 0;

  return asset;
}

/* Write data to the file system */
uint8_t asset_write(const char* file_path, void* data, uint32_t data_length) {
  FILE* f = fopen(file_path, "wb");

  if (!f) {
    return 0;
  }

  uint32_t write_length = fwrite(data, data_length, 1, f);

  fclose(f);
  if (write_length != data_length) {
    ASTERA_FUNC_DBG("mismatched write & data sizes [data_length: %i, "
                    "write_length: %i].\n",
                    data_length, write_length);
    return 0;
  }

  return 1;
}

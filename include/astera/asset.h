// TODO file writing methods (general fs, pak, zip)

/* MACROS
 * ASTERA_NO_PAK_WRITE - Disable pak file writing
 *                (best for runtime / release of game, disabled for tooling)
 */

#ifndef ASTERA_ASSET_HEADER
#define ASTERA_ASSET_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
// #include <stdio.h>

/* PAK FILE LAYOUT:
 * 0  - pack_header_t
 * 12 - entries
 * n  - start of files
 */

typedef struct {
  uint32_t       uid;
  unsigned char* data;
  uint32_t       data_length;
  const char*    name;

  int32_t chunk_start, chunk_length;

  uint8_t fs;
  uint8_t filled;
  uint8_t req;
  uint8_t req_free;
  uint8_t chunk;
} asset_t;

typedef struct {
  char     id[4];
  uint32_t count;
  uint32_t file_size;
} pak_header_t;

// -- exactly 64 bytes --
typedef struct {
  char     name[56];
  uint32_t offset;
  uint32_t size;
} pak_file_t;

// the _FILE or _MEM denoation represents the source of the data
// for when we need to push the data into the pak file itself
typedef enum {
  PAK_KEEP        = 0,
  PAK_ADD_FILE    = 1,
  PAK_ADD_MEM     = 2,
  PAK_MODIFY_FILE = 3,
  PAK_MODIFY_MEM  = 4,
  PAK_REMOVE      = 5,
} pak_change_type;

// Thing to write
typedef struct {
  char     name[56];
  uint32_t offset;
  uint32_t size;

  union {
    const char* filepath;
    asset_t     asset;
    pak_file_t* file;
  } data;

  uint8_t type;
} pak_write_t;

typedef struct {
  pak_file_t* file;

  const char* filename;

  union {
    const char* filepath;
    asset_t     asset;
  } data;

  uint8_t type;
} pak_change_t;

typedef struct {
  unsigned char* data_ptr;
  uint32_t       data_size;

  pak_file_t* entries;
  uint32_t    count;
  uint32_t    file_size;

  const char* filepath;

#if !defined(ASTERA_NO_PAK_WRITE)
  pak_change_t* changes;
  uint32_t      change_count;
  uint8_t has_change; // add / removal, will take place at close of pak header
#endif

  uint8_t file_mode; // 0 = data_ptr used, 1 = open/close files
} pak_t;

typedef struct {
  asset_t** assets;

  uint32_t count;
  uint32_t capacity;

  // uid_counter - the ID of assets in this map
  uint32_t uid_counter;

  const char* name;
  const char* filename;

  pak_t* pak;
} asset_map_t;

uint64_t asset_hash(asset_t* asset);

/* Open a file as a pak in file mode
 * file - path to the pak file
 * mode - the file mode to use with the pak file (r, rw, a)
 * return: pointer to pak_t struct for usage, fail = 0 */
pak_t* pak_open_file(const char* file);

/* Open a pak structure from memory
 * pak - the destination of the pak structure
 * data - the pointer to the data of the structure
 * data_length - the length of the data
 * returns: pointer to pak_t struct for usage, fail = 0
 * NOTE: the lifetime of this data pointer is the same as the whole
 * pak structure, don't free it if you still want to use the pak struct */
pak_t* pak_open_mem(unsigned char* data, uint32_t data_length);

#if !defined(ASTERA_NO_PAK_WRITE)
/* Output a pak structure to file
 * pak - the pak structure
 * returns: success = 1, fail = 0 */
uint8_t pak_write(pak_t* pak);

/* Write out the contents of the map to the pak file */
uint8_t asset_map_write(asset_map_t* map);
#endif

/* Close out a pak file & free all it's resources
 *  returns: success = 1, fail = 0 */
uint8_t pak_close(pak_t* pak);

/* Append a file from the system to the pak (filemode only)
 * pak - the pak file structure
 * filename - the name of the file to put in the pak file
 * returns: success = 1, fail = 0 */
uint8_t pak_add_file(pak_t* pak, const char* filename);

/* Add arbitrary memory to the pak file under a file name (filemode only)
 * pak - the pak file structure
 * name - the name of the file
 * data - the data to add
 * data_length - the length of the data
 * returns: success = 1, fail = 0 */
uint8_t pak_add_mem(pak_t* pak, const char* name, unsigned char* data,
                    uint32_t data_length);

/* Remove an entry from the pak file & it's contents (filemode only)
 * pak - pak file structure
 * entry - index of the entry
 * returns: success = 1, fail = 0 */
uint8_t pak_remove(pak_t* pak, uint32_t entry);

/* Get the data of an indexed file (allocates own space if filemode)
 * pak - the pak structure pointer
 * index - the index of the file
 * size - a pointer to int to set the size */
unsigned char* pak_extract(pak_t* pak, uint32_t index, uint32_t* size);

/* Get the data of an indexed file (uses your data pointer)
 * pak - the pak structure pointer
 * index - the index of the file
 * out - the allocated space to put the data in
 * out_cap - the capacity of the out space
 * used - the amount of used
 * returns: number of bytes used / read, fail = 0 */
uint32_t pak_extract_noalloc(pak_t* pak, uint32_t index, unsigned char* out,
                             uint32_t out_cap);

/* Find an entry in the pak file by name
 * pak - the pak structure
 * filename - the name of the entry
 * returns: index, fail = -1 */
int32_t pak_find(pak_t* pak, const char* filename);

/* Return the total number of entries in the pak structure
 * pak - pak file structure
 * returns: entry count */
uint32_t pak_count(pak_t* pak);

/* Get the true offset of a file within the pak file
 * pak - pak file structure
 * index - the index of the entry
 * returns: offset */
uint32_t pak_offset(pak_t* pak, uint32_t index);

/* Get the size of a file within the pak file
 * pak - pak file structure
 * index - the index of the entry
 * returns: size of the entry file */
uint32_t pak_size(pak_t* pak, uint32_t index);

/* Get the name of an entry by index
 * pak - the pak file structure
 * index - the index of the entry
 * returns: name string */
char* pak_name(pak_t* pak, uint32_t index);

/* Create an asset map to track assets using a pak file */
asset_map_t asset_map_create_pak(const char* filename, const char* name,
                                 uint32_t capacity);

/* Add an asset into the tracking of a map */
void asset_map_add(asset_map_t* map, asset_t* asset);
/* Remove an asset from the tracking of a map */
void asset_map_remove(asset_map_t* map, asset_t* asset);
/* Remove an asset from the tracking of a map by id */
void asset_map_removei(asset_map_t* map, uint32_t id);
/* Get a file from the asset map's directed source */
asset_t* asset_map_get(asset_map_t* map, const char* file);

/* Update for any free requests made*/
void asset_map_update(asset_map_t* map);

/* Get a file from the local system
   file - the file path of the file
   returns: formatted asset_t struct pointer with file data */
asset_t* asset_get(const char* file);

/* Get a chunk from a local system file */
asset_t* asset_get_chunk(const char* file, uint32_t chunk_start,
                         uint32_t chunk_length);

/* Free any memory used by the asset */
void asset_free(asset_t* asset);

/* Free the map and all the assets within the map */
void asset_map_free(asset_map_t* map);

/* Write data to the file system */
uint8_t asset_write(const char* file_path, void* data, uint32_t data_length);

#ifdef __cplusplus
}
#endif
#endif

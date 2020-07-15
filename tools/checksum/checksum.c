#include <astera/asset.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Invalid number of arguments passed.\n");
    return 0;
  }

  for (int i = 1; i < argc; ++i) {
    asset_t* asset = asset_get(argv[i]);
    uint64_t hash  = asset_hash(asset);
    printf("%s: %016llx\n", argv[i], hash);
    asset_free(asset);
  }

  return 0;
}

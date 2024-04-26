#include <time.h>

#include "gui.h"

#define PLATFORM_STATE_FILE_NAME_LENGTH (1024)

struct PlatformState {
  uint64_t total_size;
  void *app_memory_block;

  char binary_name[PLATFORM_STATE_FILE_NAME_LENGTH];
  char *one_past_binary_filename_slash;
};

struct platform_app_code {
  void *library_handle;

  bool32 is_valid;
  time_t library_mtime;
};

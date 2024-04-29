#include <dirent.h>
#include <dlfcn.h> // dlopen, dlsym, dlclose
#include <fcntl.h> // open, O_RDONLY
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // stat, fstat

#ifdef __APPLE__
#include <mach-o/dyld.h>
#else
#include <unistd.h> // readlink
#endif

#include <raylib.h>

#include "./hotreload.h"
#include "build.h"
#include "main.h"

struct app_memory m;
struct platform_app_code app_code;

internal void platform_get_binary_name(struct PlatformState *state) {
#ifdef __APPLE__
  uint32_t size = sizeof(state->binary_name);
  _NSGetExecutablePath(state->binary_name, &size);
#else
  readlink("/proc/self/exe", state->binary_name,
           PLATFORM_STATE_FILE_NAME_LENGTH);
#endif
  for (char *c = state->binary_name; *c; ++c) {
    if (*c == '/') {
      state->one_past_binary_filename_slash = c + 1;
    }
  }
}

internal void platform_cat_strings(size_t src_a_count, char *src_a,
                                   size_t src_b_count, char *src_b,
                                   size_t dest_count, char *dest) {
  size_t counter = 0;
  for (size_t i = 0; i < src_a_count && counter++ < dest_count; ++i) {
    *dest++ = *src_a++;
  }
  for (size_t i = 0; i < src_b_count && counter++ < dest_count; ++i) {
    *dest++ = *src_b++;
  }

  *dest++ = 0;
}

internal void platform_build_full_filename(struct PlatformState *state,
                                           char *filename, int dest_count,
                                           char *dest) {
  platform_cat_strings(
      state->one_past_binary_filename_slash - state->binary_name,
      state->binary_name, strlen(filename), filename, dest_count, dest);
}

internal void platform_load_app(struct platform_app_code *app_code,
                                char *path) {
  struct stat statbuf;
  uint32_t stat_result = stat(path, &statbuf);
  if (stat_result != 0) {
    printf("Failed to stat app code at %s", path);
    return;
  }
  app_code->library_mtime = statbuf.st_mtime;

  app_code->is_valid = 0;
  app_code->library_handle = dlopen(path, RTLD_LAZY);
  if (app_code->library_handle == 0) {
    char *error = dlerror();
    printf("Unable to load library at path %s: %s\n", path, error);
    return;
  }
  app_code->is_valid = 1;
}

internal void platform_unload_app(struct platform_app_code *app_code) {
  if (app_code->library_handle) {
    dlclose(app_code->library_handle);
    app_code->library_handle = 0;
  }
  app_code->is_valid = 0;
}

int main(int argc, char *argv[]) {
  struct PlatformState state;
  platform_get_binary_name(&state);

#ifdef HOTRELOAD
  char source_app_code_library_path[PLATFORM_STATE_FILE_NAME_LENGTH];
  char library_filename[256]; // Adjust size as necessary
  snprintf(
      library_filename, sizeof(library_filename), "lib%s.dylib",
      PROJECT_NAME); // Create the library filename based on the project name
  platform_build_full_filename(&state, library_filename,
                               sizeof(source_app_code_library_path),
                               source_app_code_library_path);
  platform_load_app(&app_code, source_app_code_library_path);
#endif

  m.PermanentStorageSize = Megabytes(64);
  m.TransientStorageSize = Megabytes(64);
  state.total_size = m.PermanentStorageSize + m.TransientStorageSize;
  m.PermanentStorage = malloc(state.total_size);
  m.TransientStorage = (uint8_t *)m.PermanentStorage + m.TransientStorageSize;
  m.IsInitialized = 0;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
  InitWindow(800, 550, "raylib example - todomvc");
  SetTargetFPS(60);

  if (!reload_libplug()) {
    return 1;
  }

  while (!WindowShouldClose()) {
#ifdef HOTRELOAD
    struct stat library_statbuf;
    stat(source_app_code_library_path, &library_statbuf);
    if (library_statbuf.st_mtime != app_code.library_mtime) {
      printf("Reloading app code\n");
      platform_unload_app(&app_code);
      platform_load_app(&app_code, source_app_code_library_path);
      if (!reload_libplug()) {
        printf("Reloading libplug\n");
        return 1;
      }
    }
#endif

    plug_update(&m);
  }

  CloseWindow();

  return 0;
}

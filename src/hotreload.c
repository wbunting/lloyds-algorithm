#include <dlfcn.h>
#include <stdio.h>

#include <raylib.h>

#include "build.h"
#include "hotreload.h"

static void *libplug = NULL;

#define PLUG(name, ...) name##_t *name = NULL;
LIST_OF_PLUGS
#undef PLUG

bool reload_libplug(void) {
  static char libplug_file_name[256];
  snprintf(libplug_file_name, sizeof(libplug_file_name), "lib%s.dylib",
           PROJECT_NAME);

  if (libplug != NULL)
    dlclose(libplug);

  libplug = dlopen(libplug_file_name, RTLD_NOW);
  if (libplug == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not load %s: %s", libplug_file_name,
             dlerror());
    return false;
  }

#define PLUG(name, ...)                                                        \
  name = dlsym(libplug, #name);                                                \
  printf("HOTRELOAD: %s\n", #name);                                            \
  if (name == NULL) {                                                          \
    TraceLog(LOG_ERROR, "HOTRELOAD: could not find %s symbol in %s: %s",       \
             #name, libplug_file_name, dlerror());                             \
    return false;                                                              \
  }
  LIST_OF_PLUGS
#undef PLUG

  return true;
}

#ifndef HOTRELOAD_H_
#define HOTRELOAD_H_

// #include <stdbool.h>

#ifndef bool
#define bool _Bool
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#include "gui.h"

#define TARGET_LINUX 0
#define TARGET_WIN64_MINGW 1
#define TARGET_WIN64_MSVC 2
#define TARGET_MACOS 3

#ifdef HOTRELOAD
#define PLUG(name, ...) extern name##_t *name;
LIST_OF_PLUGS
#undef PLUG
bool reload_libplug(void);
#else
#define PLUG(name, ...) name##_t name;
LIST_OF_PLUGS
#undef PLUG
#define reload_libplug() true
#endif // HOTRELOAD

#endif // HOTRELOAD_H_

#ifndef __FFUN_LOGS__
#define __FFUN_LOGS__
#include "config.h"

#ifdef FFUN_LOG_LEVEL_ENABLED_DEBUG
#define print_debug(...) printf(__VA_ARGS__)
#else 
#define print_debug(...)
#endif

#ifdef FFUN_LOG_LEVEL_ENABLED_VERBOSE
#define print_verbose(...) printf(__VA_ARGS__)
#else 
#define print_verbose(...)
#endif

#define print_info(...) printf(__VA_ARGS__)
#define print_error(...) printf(__VA_ARGS__)

#endif

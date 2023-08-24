#ifndef __FFUN_LOGS__
#define __FFUN_LOGS__
#include "config.h"

#ifdef FFUN_LOG_LEVEL_ENABLED_DEBUG
#define printDebug(...) printf(__VA_ARGS__)
#else 
#define printDebug(...)
#endif

#ifdef FFUN_LOG_LEVEL_ENABLED_VERBOSE
#define printVerbose(...) printf(__VA_ARGS__)
#else 
#define printVerbose(...)
#endif

#define printInfo(...) printf(__VA_ARGS__)
#define printError(...) printf(__VA_ARGS__)

#endif

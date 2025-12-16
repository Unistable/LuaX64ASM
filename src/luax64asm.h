#ifndef LUAX64ASM_H
#define LUAX64ASM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/lua.h"
#include "../include/lauxlib.h"
#include "../include/lualib.h"

#ifdef _WIN32
#define LUAX64ASM_API __declspec(dllexport)
#else
#define LUAX64ASM_API
#endif

#ifdef __cplusplus
}
#endif

#endif
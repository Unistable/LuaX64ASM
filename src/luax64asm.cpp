#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstring>
#include <cstdint>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

// === Совместимость со старыми версиями Lua ===
#ifndef LUA_VERSION_NUM
#error "Невозможно определить версию Lua"
#endif

#if LUA_VERSION_NUM < 503
#define lua_pushinteger(L, n) lua_pushnumber((L), (lua_Number)(n))
#define luaL_checkinteger(L, n) ((lua_Integer)luaL_checknumber((L), (n)))
#endif

#if LUA_VERSION_NUM < 504
static void* compat_lua_newuserdatauv(lua_State* L, size_t sz, int) {
    (void)sz; (void)L;
    return lua_newuserdata(L, sz);
}
#define lua_newuserdatauv compat_lua_newuserdatauv
#endif
// ===========================================

struct asm_block_t {
    void* ptr;
    size_t size;
    bool finalized;
};

static int asm_block_alloc(lua_State* L) {
    lua_Integer size_int = luaL_checkinteger(L, 1);
    if (size_int <= 0) return luaL_error(L, "size must be > 0");
    if (size_int > 1024 * 1024) return luaL_error(L, "block too large");
    size_t size = (size_t)size_int;

    void* mem = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) return luaL_error(L, "VirtualAlloc failed");

    asm_block_t* b = (asm_block_t*)lua_newuserdatauv(L, sizeof(asm_block_t), 0);
    b->ptr = mem;
    b->size = size;
    b->finalized = 0;
    luaL_setmetatable(L, "luax64asm.block");
    return 1;
}

static int asm_block_write(lua_State* L) {
    asm_block_t* b = (asm_block_t*)luaL_checkudata(L, 1, "luax64asm.block");
    if (b->finalized) return luaL_error(L, "block already finalized");

    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    if (len > b->size) return luaL_error(L, "data exceeds block size");

    memcpy(b->ptr, data, len);
    lua_pushboolean(L, 1);
    return 1;
}

static int asm_block_exec(lua_State* L) {
    asm_block_t* b = (asm_block_t*)luaL_checkudata(L, 1, "luax64asm.block");
    if (b->finalized) return luaL_error(L, "block already executable");

    DWORD old_prot = 0;
    if (!VirtualProtect(b->ptr, b->size, PAGE_EXECUTE_READ, &old_prot)) {
        VirtualFree(b->ptr, 0, MEM_RELEASE);
        b->ptr = 0;
        return luaL_error(L, "VirtualProtect failed");
    }

    FlushInstructionCache(GetCurrentProcess(), b->ptr, b->size);
    b->finalized = 1;
    lua_pushboolean(L, 1);
    return 1;
}

static int asm_block_call_i64(lua_State* L) {
    asm_block_t* b = (asm_block_t*)luaL_checkudata(L, 1, "luax64asm.block");
    if (!b->finalized) return luaL_error(L, "block not executable");
    int64_t arg = (int64_t)luaL_checkinteger(L, 2);
    int64_t(*fn)(int64_t) = (int64_t(*)(int64_t))(b->ptr);
    lua_pushinteger(L, (lua_Integer)fn(arg));
    return 1;
}

static int asm_block_call_i64_i64(lua_State* L) {
    asm_block_t* b = (asm_block_t*)luaL_checkudata(L, 1, "luax64asm.block");
    if (!b->finalized) return luaL_error(L, "block not executable");
    int64_t a1 = (int64_t)luaL_checkinteger(L, 2);
    int64_t a2 = (int64_t)luaL_checkinteger(L, 3);
    int64_t(*fn)(int64_t, int64_t) = (int64_t(*)(int64_t, int64_t))(b->ptr);
    lua_pushinteger(L, (lua_Integer)fn(a1, a2));
    return 1;
}

static int asm_block_call_i64_i64_i64(lua_State* L) {
    asm_block_t* b = (asm_block_t*)luaL_checkudata(L, 1, "luax64asm.block");
    if (!b->finalized) return luaL_error(L, "block not executable");
    int64_t a1 = (int64_t)luaL_checkinteger(L, 2);
    int64_t a2 = (int64_t)luaL_checkinteger(L, 3);
    int64_t a3 = (int64_t)luaL_checkinteger(L, 4);
    int64_t(*fn)(int64_t, int64_t, int64_t) = (int64_t(*)(int64_t, int64_t, int64_t))(b->ptr);
    lua_pushinteger(L, (lua_Integer)fn(a1, a2, a3));
    return 1;
}

static int asm_block_call_void(lua_State* L) {
    asm_block_t* b = (asm_block_t*)luaL_checkudata(L, 1, "luax64asm.block");
    if (!b->finalized) return luaL_error(L, "block not executable");
    void(*fn)(void) = (void(*)(void))(b->ptr);
    fn();
    return 0;
}

static int asm_block_gc(lua_State* L) {
    asm_block_t* b = (asm_block_t*)luaL_checkudata(L, 1, "luax64asm.block");
    if (b->ptr) {
        VirtualFree(b->ptr, 0, MEM_RELEASE);
        b->ptr = 0;
    }
    return 0;
}

static const luaL_Reg asm_block_methods[] = {
    {"call_i64",         asm_block_call_i64},
    {"call_i64_i64",     asm_block_call_i64_i64},
    {"call_i64_i64_i64", asm_block_call_i64_i64_i64},
    {"call_void",        asm_block_call_void},
    {"__gc",             asm_block_gc},
    {0, 0}
};

static const luaL_Reg lib_functions[] = {
    {"alloc", asm_block_alloc},
    {"write", asm_block_write},
    {"exec",  asm_block_exec},
    {0, 0}
};

extern "C" __declspec(dllexport) int luaopen_luax64asm(lua_State* L) {
    // Создаём и настраиваем метатаблицу
    luaL_newmetatable(L, "luax64asm.block");
    luaL_newlib(L, asm_block_methods);      // создаёт таблицу с методами
    lua_setfield(L, -2, "__index");         // mt.__index = {методы}
    lua_pop(L, 1);                          // убираем метатаблицу

    // Экспортируем основные функции
    luaL_newlib(L, lib_functions);
    return 1;
}
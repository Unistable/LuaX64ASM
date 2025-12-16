# LuaX64ASM — Execute x86-64 Machine Code from Lua on Windows

## Requirements
- Windows 10 (x64)
- Microsoft Visual Studio (for `cl.exe`)
- LuaJIT 2.1 (headers in `./include/`)

## Build
Run `build.bat`. Output: `lua/luax64asm.dll`

## Usage
```lua
local asm = require("luax64asm")

-- Example: add two numbers
local code = string.char(
    0x48, 0x89, 0xCA, -- mov rdx, rcx
    0x48, 0x01, 0xD0, -- add rax, rdx
    0xC3              -- ret
)

local fn = asm.new(code)
print(fn:call_i64_i64(100, 42)) --> 142

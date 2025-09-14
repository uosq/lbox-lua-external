# LUA Runner

### You can use this to run luas without having to open Lmaobox's LUA tab

---

### Requirements

Runtime requirements:

+ QT5/6
+ Lmaobox with helper script loaded

---

Build requirements:
+ QT5/6
+ CMake
+ Clang/GCC

Build instructions:

```sh
git clone https://github.com/uosq/lbox-lua-external.git
cd lbox-lua-external
mkdir build
mkdir build/Desktop-Release
cmake --build build/Desktop-Release --target all
```

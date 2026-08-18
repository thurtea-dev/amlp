# amlp

A C++20 LPC game driver targeting FluffOS/LDMud/DGD dialect compatibility: lexer, parser, bytecode VM, and a TCP server for running an LPC mudlib.

## Build and run

```
cmake -B build -S .
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

```
./build/amlp etc/driver.cfg
```

boots the driver's own bundled mudlib under `mudlib/`.

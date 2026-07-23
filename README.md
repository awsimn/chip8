# CHIP-8 Emulator

A C++ implementation of a CHIP-8 virtual machine emulator using SDL2 for windowing, rendering, and keyboard input.

## Features

- CHIP-8 fetch / decode / execute cycle
- 64 x 32 monochrome framebuffer
- SDL2 window rendering
- ROM loading from the command line
- CHIP-8 keypad input handling
- Basic timing control for CPU execution

## Project Files

- `Chip8.h` - Core emulator class definition
- `Chip8.cpp` - CHIP-8 CPU implementation and opcode handlers
- `Platform.h` - SDL2 platform interface
- `Platform.cpp` - SDL2 window, rendering, and input handling
- `main.cpp` - Program entry point
- `README.md` - Project overview and usage

## Requirements

- C++ compiler with C++17 support
- SDL2 development libraries

## Build

Compile all source files and link against SDL2:

```bash
g++ main.cpp Chip8.cpp Platform.cpp -o Amulator -lSDL2
```

## Run

The emulator expects three command-line arguments:

```bash
./Amulator <Scale> <Delay> <ROM>
```

- `Scale` - Window scale factor
- `Delay` - Delay in milliseconds between CPU cycles
- `ROM` - Path to the CHIP-8 ROM file

Example:

```bash
./Amulator 10 2 roms/PONG
```

## Keyboard Controls

The emulator uses the standard CHIP-8 keypad mapping:

```text
1 2 3 C
4 5 6 D
7 8 9 E
A 0 B F
```

Mapped to the physical keyboard like this:

```text
1 2 3 4
q w e r
a s d f
z x c v
```

## Notes

- The ROM must be a valid CHIP-8 binary.
- The emulator window can be closed with the window close button or `Esc`.
- Some ROMs may require precise timing, so you may need to adjust the delay value.


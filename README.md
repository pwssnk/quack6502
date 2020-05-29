# quack6502
 An (unfinished) emulator suite for MOS6502 based systems. Currently only supports NES.

## Background
This is a project I started a while ago to gain some familiarity with C++, having previously only worked with higher level languages like Python and Java. I figured that writing an emulator for the 6502 microprocessor would be a good way to to get down to the nitty-gritty. After implementing the CPU (illegal opcodes notwithstanding), I thought it would be fun to actually do something with it, so I built an NES emulator on top of it. I had originally intended to implement emulators for a variety of other 6502 based systems as well, but I lost interest. With this in mind the emulator components were designed to be sort of modular. The code is pretty jank though; it's been a learning experience.

## Dependencies

Emulation and rendering are implemented as seperate components. The emulator library, `qk-emulator`, has no dependencies beyond the C++ Standard Library. The renderer, `qk-renderer`, handles audiovisual output and player input and requires [SDL2](https://www.libsdl.org/download-2.0.php).

## Usage (NES)
There is currently only limited support for the Nintendo Entertainment System:

* No audio, as the APU code is still too buggy to be usable
* Support for NTSC only
* Support for NROM cartrigde based games only (iNES mapper 0)

Usage is as follows:
```
qk [path to iNES ROM file]
```

Controls are hardcoded as follows:

| NES gamepad  | Keyboard    |
| ------------ | ----------- |
| D-pad        | Arrow keys  |
| A            | Z           |
| B            | X           |
| Start        | Enter       |
| Select       | Right shift |

## Copyright and license
(c) 2019-2020 pwssnk -- code available under GPLv3 license.

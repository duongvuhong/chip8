# Table of Contents

* [Install](#Install)
    * [Dependencies](#Dependencies)
    * [Compile](#Compile)
    * [Run](#Run)
* [CHIP8](#CHIP8)
    * [About](#About)
    * [Memory](#Memory)
    * [Registers](#Registers)
    * [Keyboard](#Keyboard)
    * [Display](#Display)
    * [Timer and Sound](#Timer-and-Sound)
    * [Instructions](#Instructions)
* [TODO](#TODO)

# Install
## Dependencies
* Linux: freeglut3-dev(Debian-based distro), freeglut-devel-3*(Redhat-based distro)
* Windows: not support for now
* MacOS: not test but should work with freeglut3 dev package installed
## Compile
```
$ make
```
## Run
```
$ ./emulator roms/games/Space\ Invaders\ \[David\ Winter\].ch8
```

_Please read introduction txt file according to a ROM for more details. Enjoy ^_^!!!_

```
         CHIP8                            PC Keyboard
| - | - | - | - | - | - |       | - | - | - | - | - | - |
| * | 1 | 2 | 3 | C | * |       | * | 1 | 2 | 3 | 4 | * |
| * | 4 | 5 | 6 | D | * |   ->  | * | Q | W | E | R | * | 
| * | 7 | 8 | 9 | E | * |       | * | A | S | D | F | * |
| * | A | 0 | B | F | * |       | * | Z | X | C | V | * |
| - | - | - | - | - | - |       | - | - | - | - | - | - |
```

# CHIP8
## About
CHIP8 is a simple, interpreted, programming language which was first used on some do-it-yourself computer systems in the late 1970s and early 1980s. The COSMAC VIP, DREAM 6800, and ETI computers are a few examples. These computers typically were designed to use a television as a display, had between 1 and 4K of RAM, and used a 16-key hexadecimal keypad for input.

In the early 1990s, the CHIP-8 language was revived by a man named Andreas Gustafsson. He created a CHIP-8 interpreter for the HP graphing calculator, called CHIP-48. The HP48 was lacking a way to easily make fast games at the time, and CHIP-8 was the answer. CHIP-8 later begat Super CHIP-48, a modification of CHIP-8 which allowed higher resolution graphics, as well as other graphical enhancements.

## Memory
The CHIP-8 language is capable of accessing up to 4KB (4096 bytes) of RAM, from location 0x000 to 0xFFF. The first 512 bytes, from 0x000 to 0x1FF, are where the original interpreter was located, and should not be used by programs.

Most CHIP-8 programs start at location 0x200, but some begin at 0x600. Programs beginning at 0x600 are intended for the ETI 660 computer.

**Memory Map:**
```
+---------------+= 0xFFF (4095) End of CHIP-8 RAM
|               |
|               |
|               |
|               |
|               |
| 0x200 to 0xFFF|
|     CHIP-8    |
| Program / Data|
|     Space     |
|               |
|               |
|               |
+- - - - - - - -+= 0x600 (1536) Start of ETI 660 CHIP-8 programs
|               |
|               |
|               |
+---------------+= 0x200 (512) Start of most CHIP-8 programs
| 0x000 to 0x1FF|
| Reserved for  |
|  interpreter  |
+---------------+= 0x000 (0) Start of CHIP-8 RAM`
```

## Registers
CHIP-8 has 16 general purpose 8-bit registers, usually referred to as Vx, where x is a hexadecimal digit (0 through F). The 16th register is used for the 'carry flag'.

There is also a 16-bit register called I. This register is generally used to store memory addresses, so only the lowest (rightmost) 12 bits are usually used.

Super CHIP-48, which was used on the HP-48 calculators, contains 8 8-bit user-flag registers: R0-R7. They can't be directly used, but register V0-V7 can be saved to-and loaded from them.

CHIP-8 also has two special purpose 8-bit registers, for the delay and sound timers. When these registers are non-zero, they are automatically decremented at a rate of 60Hz.

There are also some "pseudo-registers" which are not accessable from CHIP-8 programs. The program counter (PC) should be 16-bit, and is used to store the currently executing address. The stack pointer (SP) can be 8-bit, it is used to point to the topmost level of the stack.

The stack is an array of 16 16-bit values, used to store the address that the interpreter shoud return to when finished with a subroutine. CHIP-8 allows for up to 16 levels of nested subroutines.

## Keyboard
The computers which originally used the CHIP-8 Language had a 16-key hexadecimal keypad with the following layout:

| - | - | - | - | - | - |
| - |---|---|---|---| - |
| * | 1 | 2 | 3 | C | * |
| * | 4 | 5 | 6 | D | * |
| * | 7 | 8 | 9 | E | * |
| * | A | 0 | B | F | * |
| - | - | - | - | - | - |

This layout must be mapped into various other configurations to fit the keyboards of today's platforms.

## Display
The original implementation of the CHIP-8 language used a 64x32-pixel monochrome display with this format:
```
=================
|(0,0)   (63,0) |
|(0,31)  (63,31)|
=================
```
Recently, Super CHIP-48 added a 128x64-pixel extend mode.
```
=================
|(0,0)   (127,0) |
|(0,63)  (127,63)|
=================
```

CHIP-8 draws graphics on screen through the use of sprites. A sprite is a group of bytes which are a binary representation of the desired picture. CHIP-8 sprites may be up to 15 bytes, for a possible sprite size of 8x15. A sprite of 16x16 size is available in extend mode.

Programs may also refer to a group of sprites representing the hexadecimal digits 0 through F. These sprites are 5 bytes long, or 8x5 pixels. The data should be stored in the interpreter area of CHIP-8 memory (0x000 to 0x1FF).

**"0" character in binary and hexadecimal**
```
--------------------------
| Char | Bin      | Hex  |
|------|----------|------|
| **** | 11110000 | 0xF0 |
| *  * | 10010000 | 0x90 |
| *  * | 10010000 | 0x90 |
| *  * | 10010000 | 0x90 |
| **** | 11110000 | 0xF0 |
--------------------------
```

## Timer and Sound
CHIP-8 provides 2 timers, a delay timer and a sound timer.

The delay timer is active whenever the delay timer register (DT) is non-zero. This timer does nothing more than subtract 1 from the value of DT at a rate of 60Hz. When DT reaches 0, it deactivates.

The sound timer is active whenever the sound timer register (ST) is non-zero. This timer also decrements at a rate of 60Hz, however, as long as ST's value is greater than zero, the CHIP-8 buzzer will sound. When ST reaches zero, the sound timer deactivates.

The sound produced by the CHIP-8 interpreter has only one tone. The frequency of this tone is decided by the author of the interpreter.

## Instructions
The original implementation of the CHIP-8 language includes 36 different instructions, including math, graphics, and flow control functions.

Super CHIP-48 added an additional	10 instructions, for a total of 46.

All instructions are 2 bytes long and are stored most-significant-byte first. In memory, the first byte of each instruction should be located at an even addresses. If a program includes sprite data, it should be padded so any instructions following it will be properly situated in RAM.

This document does not yet contain descriptions of the Super CHIP-48 instructions. They are, however, listed below.

In these listings, the following variables are used:
```
nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
n or nibble - A 4-bit value, the lowest 4 bits of the instruction
x - A 4-bit value, the lower 4 bits of the high byte of the instruction
y - A 4-bit value, the upper 4 bits of the low byte of the instruction
kk or byte - An 8-bit value, the lowest 8 bits of the instruction
```

**[CHIP-8 Instructions Table](https://en.wikipedia.org/wiki/CHIP-8)**

**Super CHIP-48 Addition Instructions**
```
00CN*    Scroll display N lines down
00FB*    Scroll display 4 pixels right
00FC*    Scroll display 4 pixels left
00FD*    Exit CHIP interpreter
00FE*    Disable extended screen mode
00FF*    Enable extended screen mode for full-screen graphic
DXYN*    Show N-byte sprite from M(I) at coords (VX,VY), VF :=
         collision. If N=0 and extended mode, show 16x16 sprite.
FX30*    Point I to 10-byte font sprite for digit VX (0..9)
FX75*    Store V0..VX in RPL user flags (X <= 7)
FX85*    Read V0..VX from RPL user flags (X <= 7)
```
[For more details about Super CHIP-48](http://devernay.free.fr/hacks/chip8/schip.txt)

# TODO
* Fix: many ROM don't work.
* Support: Super CHIP-48 instructions
#ifndef _CHIP8_H
#define _CHIP8_H

#include <stdint.h>

#define INTERPRETER_SPACE 512
#define CHIP8_MEMORY_SIZE 4096

#define V_REGISTERS 16
#define STACK_LEVEL 16
#define KEYPAD 16

#ifdef SUPPORT_SCHIP_48
#define XSCREEN_WIDTH 128
#define XSCREEN_HEIGHT 64
#define XSCREEN_PIXELS (XSCREEN_WIDTH * XSCREEN_HEIGHT)

#define RPL_USER_FLAGS 16

#endif

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_PIXELS (SCREEN_WIDTH * SCREEN_HEIGHT)

struct chip8 {
	uint16_t opcode;
	uint8_t memory[CHIP8_MEMORY_SIZE];
	uint8_t reg8[V_REGISTERS];
	uint16_t reg16;
	uint16_t pc;
	uint16_t stack[STACK_LEVEL];
	uint16_t sp;
	uint8_t gfx[SCREEN_PIXELS];
	uint8_t delay_timer;
	uint8_t sound_timer;
	uint8_t key[KEYPAD];
	uint8_t compa;
#ifdef SUPPORT_SCHIP_48
	uint8_t xgfx[XSCREEN_PIXELS];
	uint8_t rpl[RPL_USER_FLAGS];
	uint8_t extend;
#endif
	uint8_t draw_flag;
};

extern void chip8_init(struct chip8 *);
extern int chip8_load(struct chip8 *, const char *);
extern void chip8_emulate_cycle(struct chip8 *);

#endif

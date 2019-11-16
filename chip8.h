#ifndef _CHIP8_H
#define _CHIP8_H

#include <stdint.h>

#define CHIP8_MEMORY_SIZE       4096
#define CHIP8_INTERPRETER_SPACE 512

#define CHIP8_V_REG_NUMBER 16

#define CHIP8_STACK_LEVEL 16

#define CHIP8_KEY_NUMBER 16

#define CHIP8_SCREEN_WIDTH  64 /* in pixels */
#define CHIP8_SCREEN_HEIGHT 32 /* in pixels */
#define CHIP8_SCREEN_PIXELS (CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT)

#ifdef SUPPORT_SCHIP_48
#define SCHIP48_SCREEN_WIDTH  128
#define SCHIP48_SCREEN_HEIGHT 64
#define SCHIP48_SCREEN_PIXELS (XSCREEN_WIDTH * XSCREEN_HEIGHT)

#define RPL_USER_FLAGS 8
#endif

typedef struct {
	uint16_t opcode;
	uint8_t memory[CHIP8_MEMORY_SIZE];
	uint8_t V[CHIP8_V_REG_NUMBER];
	uint16_t I;
	uint16_t pc;
	uint16_t stack[CHIP8_STACK_LEVEL];
	uint16_t sp;
#ifdef SUPPORT_SCHIP_48
	uint8_t gfx[SCHIP48_SCREEN_PIXELS];
	uint8_t R[RPL_USER_FLAGS];
	uint8_t extend;
#else
	uint8_t gfx[CHIP8_SCREEN_PIXELS];
#endif
	uint8_t delay_timer;
	uint8_t sound_timer;
	uint8_t key[CHIP8_KEY_NUMBER];
	uint8_t compa;
	uint8_t draw_flag;
} chip8_t;

#ifdef SUPPORT_SCHIP_48
#define GFX_INDEX(r, c) ((r) * SCHIP48_SCREEN_WIDTH + (c))
#else
#define GFX_INDEX(r, c) ((r) * CHIP8_SCREEN_WIDTH + (c))
#endif

extern void chip8_init(chip8_t *);
extern int chip8_load(chip8_t *, const char *);
extern void chip8_emulate_cycle(chip8_t *);
extern void chip8_tick(chip8_t *);

#endif /* _CHIP8_H */
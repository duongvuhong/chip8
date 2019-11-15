#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "chip8.h"

#define UNKNOWN_OPCODE(op, kk) \
do {                                                         \
	CC_ERROR("Unknown: opcode(0x%x) kk(0x%02x)\n", op, kk);  \
	exit(42);                                                \
} while (0)

#define opcode       (chip->opcode)
#define memory       (chip->memory)
#define V            (chip->V)
#define flag         (V[0xF])
#define I            (chip->I)
#define pc           (chip->pc)
#define stack        (chip->stack)
#define sp           (chip->sp)
#define compa        (chip->compa)
#define delay_timer  (chip->delay_timer)
#define sound_timer  (chip->sound_timer)
#define key          (chip->key)
#define gfx          (chip->gfx)
#define draw_flag    (chip->draw_flag)

#define FONTSET_BYTES_PER_CHAR 5

static const char chip8_fontset[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
	0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
	0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
	0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
	0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
	0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
	0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
	0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
	0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
	0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
	0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
	0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
	0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
	0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
	0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
	0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

static inline void clear_screen(chip8_t *chip)
{
	memset(gfx, 0, CHIP8_SCREEN_PIXELS * sizeof(uint8_t));
	draw_flag = TRUE;
}

static void draw_sprite(chip8_t *chip, uint8_t x, uint8_t y, uint8_t h)
{
	flag = 0;

	for (unsigned byte_index = 0; byte_index < h; byte_index++) {
		uint8_t byte = memory[I + byte_index];

		for (unsigned bit_index = 0; bit_index < 8; bit_index++) {
			/* the value of the bit in the sprite */
			uint8_t bit = (byte >> bit_index) & 0x01;
			/* the value of the current pixel on the screen */
			uint8_t *pixel = &gfx[GFX_INDEX((y + byte_index) % CHIP8_SCREEN_HEIGHT, (x + (7 - bit_index)) % CHIP8_SCREEN_WIDTH)];
			
			/* if drawing to the screen would cause any pixel to be erased, set flag to 1 */
			if (bit == 1 && *pixel == 1)
				flag = 1;
			
			/* draw this pixel */
			*pixel ^= bit;
		}
	}

	draw_flag = TRUE;
}

void chip8_init(chip8_t *chip)
{
	memset(chip, 0, sizeof(chip8_t));

	pc = 0x200;
	compa = FALSE;
	draw_flag = TRUE;
	memcpy(memory, chip8_fontset, 80 * sizeof(char));

	srand(time(NULL));
}

int chip8_load(chip8_t *chip, const char *rom)
{
	size_t total = 0;
	uint8_t buffer[100];
	FILE *fp;

	fp = fopen(rom, "rb");
	RETURN_WITH_ERRMSG_IF(!fp, -1, "Failed to open (%s) rom file!\n", rom);

	for (; total + CHIP8_INTERPRETER_SPACE < CHIP8_MEMORY_SIZE;) {
		size_t nread = fread(buffer, sizeof(uint8_t), 100, fp);
		RETURN_WITH_ERRMSG_IF(ferror(fp), -1, "Failed to read (%s) rom file!\n", rom);
		if (nread == 0)
			break;

		memcpy(memory + CHIP8_INTERPRETER_SPACE + total, buffer, nread);
		total += nread;
	}

	fclose(fp);

	RETURN_WITH_ERRMSG_IF((total + CHIP8_INTERPRETER_SPACE) >= CHIP8_MEMORY_SIZE, -1,
						  "Not enough space for %s (>>%lu bytes)!\n", rom, total);

	CC_INFO("Loaded %s (%lu bytes)\n", rom, total);

	return 0;
}

void chip8_emulate_cycle(chip8_t *chip)
{
	opcode = memory[pc] << 8 | memory[pc + 1];

#define addr (opcode & 0x0FFF)
#define kk   (opcode & 0x00FF)
#define n    (opcode & 0x000F)
#define x    ((opcode & 0x0F00) >> 8)
#define y    ((opcode & 0x00F0) >> 4)

#define reposition_pc(n) pc = n

	/* decodes opcodes of CHIP-8 and SCHIP-48*/
	switch (opcode & 0xF000) {
	case 0x0000:
		switch (kk) {
		case 0x00E0: /* 00E0 - CLS */
			clear_screen(chip);
			reposition_pc(pc + 2);
			break;
		case 0x00EE: /* 00EE - RET */
			reposition_pc(stack[--sp]);
			break;
		case 0x00FA: /* 00FA - [EMU] turn on copatibility mode */
			compa = TRUE;
			reposition_pc(pc + 2);
			break;
		default:
			UNKNOWN_OPCODE(opcode, kk);
		}
		break;
	case 0x1000: /* 1NNN - JP addr */
		reposition_pc(addr);
		break;
	case 0x2000: /* 2NNN - CALL addr */
		stack[sp++] = pc + 2;
		ASSERT_WITH_ERRMSG_IF(sp >= CHIP8_STACK_LEVEL, 1, "Out of stack\n");
		reposition_pc(addr);
		break;
	case 0x3000: /* 3XNN - SE Vx, byte */
		reposition_pc(pc + (V[x] == kk ? 4 : 2));
		break;
	case 0x4000: /* 4XNN - SNE Vx, byte */
		reposition_pc(pc + (V[x] != kk ? 4 : 2));
		break;
	case 0x5000: /* 5XY0 - SE Vx, Vy */
		reposition_pc(pc + (V[x] == V[y] ? 4 : 2));
		break;
	case 0x6000: /* 6XNN - LD Vx, byte */
		V[x] = kk;
		reposition_pc(pc + 2);
		break;
	case 0x7000: /* 7XNN - ADD Vx, byte */
		V[x] += kk;
		reposition_pc(pc + 2);
		break;
	case 0x8000:
		switch (n) {
		case 0x00: /* 8XY0 - LD Vx, Vy */
			V[x] = V[y];
			break;
		case 0x01: /* 8XY1 - OR Vx, Vy */
			V[x] |= V[y];
			break;
		case 0x02: /* 8XY2 - AND Vx, Vy */
			V[x] &= V[y];
			break;
		case 0x03: /* 8XY3 - XOR Vx, Vy */
			V[x] ^= V[y];
			break;
		case 0x04: /* 8XY4 - ADD Vx, Vy */
			flag = V[y] > (0xFF - V[x]) ? 1 : 0; /* Check if there's a carry */
			V[x] += V[y];
			break;
		case 0x05: /* 8XY5 - SUB Vx, Vy */
			flag = V[x] > V[y] ? 1 : 0; /* Check if there's a borrow */
			V[x] -= V[y];
			break;
		case 0x06: /* 8XY6 - SHR Vx {, Vy} */
			flag = V[x] & 0x01;
			V[x] >>= 1;
			break;
		case 0x07: /* 8XY7 - SUBN Vx, Vy */
			flag = V[y] > V[x] ? 1 : 0; /* Check if there's a borrow */
			V[x] = V[y] - V[x];
			break;
		case 0x0E: /* 8XYE - SHL Vx {, Vy} */
			flag = (V[x] >> 7) & 0x01;
			V[x] <<= 1;
			break;
		default:
			UNKNOWN_OPCODE(opcode, kk);
		}
		reposition_pc(pc + 2);
		break;
	case 0x9000: /* 9XY0 - SNE Vx, Vy */
		switch (n) {
		case 0x00:
			reposition_pc(pc + (V[x] != V[y] ? 4 : 2));
			break;
		default:
			UNKNOWN_OPCODE(opcode, kk);
		}
		break;
	case 0xA000: /* ANNN - LD I, addr */
		I = addr;
		reposition_pc(pc + 2);
		break;
	case 0xB000: /* BNNN - JP V0, addr */
		reposition_pc(V[0] + addr);
		break;
	case 0xC000: /* CXNN - RND Vx, byte */
		V[x] = RANDOM_BYTE & kk;
		reposition_pc(pc + 2);
		break;
	case 0XD000: /* DXYN - DRW Vx, Vy, nibble */
		draw_sprite(chip, V[x], V[y], n);
		reposition_pc(pc + 2);
		break;
	case 0xE000: /* key-pressed events */
		switch (kk) {
		case 0x009E: /* EX9E - SKP Vx */
			reposition_pc(pc + (key[V[x]] ? 4 : 2));
			break;
		case 0x00A1: /* EXA1 - SKNP Vx */
			reposition_pc(pc + (!key[V[x]] ? 4 : 2));
			break;
		default:
			UNKNOWN_OPCODE(opcode, kk);
		}
		break;
	case 0xF000: /* misc */
		switch (kk) {
		case 0x0007: /* FX07 - LD Vx, DT */
			V[x] = delay_timer;
			break;
		case 0x000A: /* FX0A - LD Vx, K */
			printf("Waiting for key instruction\n");
			while (TRUE) {
				for (int i = 0; i < CHIP8_KEY_NUMBER; i++) {
					if (key[i]) {
						V[x] = i;
						goto key_pressed;
					}
				}
			}
key_pressed:
			printf("A key is pressed");
			break;
		case 0x0015: /* FX15 - LD DT, Vx */
			delay_timer = V[x];
			break;
		case 0x0018: /* FX18 - LD ST, Vx */
			sound_timer = V[x];
			break;
		case 0x001E: /* FX1E - ADD I, Vx */
			/* Check if there's a overflow */
			flag = I + V[x] > 0xFFF ? 1 : 0;
			I += V[x];
			break;
		case 0x0029: /* FX29 - LD F, Vx */
			I = V[x] * FONTSET_BYTES_PER_CHAR;
			break;
		case 0x0033: /* FX33 - LD B, Vx */
			memory[I]     = (V[x] / 100) % 10;
			memory[I + 1] = (V[x] / 10) % 10;
			memory[I + 2] = (V[x] / 1) % 10;
			break;
		case 0x0055: /* FX55 - LD [I], Vx */
			memcpy(memory + I, V, x + 1);
			if (compa)
				I += x + 1;
			break;
		case 0x0065: /* FX65 - LD Vx, [I] */
			memcpy(V, memory + I, x + 1);
			if (compa)
				I += x + 1;
			break;
		default:
			UNKNOWN_OPCODE(opcode, kk);
		}
		reposition_pc(pc + 2);
		break;
	default:
		UNKNOWN_OPCODE(opcode, kk);
		break;
	}
}

void chip8_tick(chip8_t *chip)
{
	if (delay_timer > 0)
		--delay_timer;

	if (sound_timer > 0) {
		--sound_timer;
		if (sound_timer == 0)
			printf("BEEP!\n");
	}
}
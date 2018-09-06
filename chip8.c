#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chip8.h"

#define OPCODE_GET_NNN(op) ((op) & 0x0FFF) /* address */
#define OPCODE_GET_0NN(op) ((op) & 0x00FF) /* 8-bit constant */
#define OPCODE_GET_00N(op) ((op) & 0x000F) /* 4-bit constant */

#define OPCODE_GET_X_ID(op) (((op) & 0x0F00) >> 8) /* 4-bit x register identifier */
#define OPCODE_GET_Y_ID(op) (((op) & 0x00F0) >> 4) /* 4-bit y register identifier */

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

static inline void clear_screen(struct chip8 *chip)
{
	memset(chip->gfx, 0, SCREEN_PIXELS * sizeof(uint8_t));
	chip->draw_flag = TRUE;
}

static void draw_sprite(struct chip8 *chip, uint8_t x, uint8_t y, uint8_t h)
{
	uint8_t p;
	uint8_t *flag = chip->reg8 + 0xF;
	uint8_t *gfx = chip->gfx;
	const uint8_t *memory = chip->memory;
	const uint16_t addr = chip->reg16;

	*flag = 0;

	for (int r = 0; r < h; r++) {
		p = memory[addr + r];
		for (int n = 0; n < 8; n++) {
			if ((p & (0x80 >> n)) != 0) {
				if (gfx[(y + r) * SCREEN_WIDTH + (x + n)] == 1)
					*flag = 1;
				gfx[(y + r) * SCREEN_WIDTH + (x + n)] ^= 1;
			}
		}
	}

	chip->draw_flag = TRUE;
}

void chip8_init(struct chip8 *chip)
{
	chip->pc = 0x200; /* program counter starts at 0x200 */
	chip->opcode = 0; /* reset current opcode */
	chip->reg16 = 0;  /* reset register address */
	chip->sp = 0;     /* reset stack pointer */

	/* clear display */
	clear_screen(chip);
	/* clear stack */
	memset(chip->stack, 0, STACK_LEVEL * sizeof(uint16_t));
	/* clear registers V0-VF */
	memset(chip->reg8, 0, V_REGISTERS * sizeof(uint8_t));
	/* clear keypad */
	memset(chip->key, 0, KEYPAD * sizeof(uint8_t));
	/* clear memory */
	memset(chip->memory, 0, EMULATOR_MEMORY_SIZE * sizeof(uint8_t));

	/* load fontset */
	memcpy(chip->memory, chip8_fontset, 80 * sizeof(uint8_t));

	/* reset timers */
	chip->delay_timer = 0;
	chip->sound_timer = 0;
}

int chip8_load(struct chip8 *chip, const char *rom)
{
	size_t n;
	size_t total = 0;
	uint8_t buf[100];
	FILE *fp;

	fp = fopen(rom, "rb");
	if (!fp) {
		CC_ERROR("Failed to open %s rom file!\n", rom);
		return -1;
	}

	while ((n = fread(buf, 1, 100, fp)) != 0
		   && (total + INTERPRETER_SPACE) < EMULATOR_MEMORY_SIZE) {
		memcpy(chip->memory + INTERPRETER_SPACE + total, buf, n);
		total += n;
	}

	fclose(fp);

	if ((total + INTERPRETER_SPACE) >= EMULATOR_MEMORY_SIZE) {
		CC_ERROR("Not enough space for %s (>>%lu bytes)!\n", rom, total);
		return -1;
	}

	CC_INFO("%s is %lu byte(s) in size\n", rom, total);

	return 0;
}

void chip8_emulate_cycle(struct chip8 *chip)
{
	int pressed = FALSE;
	uint8_t *V = chip->reg8;
	uint8_t *memory = chip->memory;
	uint16_t *addr = &chip->reg16;
	uint16_t *pc = &chip->pc;
	uint16_t *stack = chip->stack;
	uint16_t *sp = &chip->sp;
	uint8_t *delay = &chip->delay_timer;
	uint8_t *sound = &chip->sound_timer;
	const uint8_t *key = chip->key;

	const uint16_t opcode = chip->opcode = memory[*pc] << 8 | memory[*pc + 1];
	/* decodes 35 opcodes of CHIP-8 */
	switch (opcode & 0xF000) {
	case 0x0000:
		switch (OPCODE_GET_00N(opcode)) {
		case 0x0000: /* 00E0 - CLS */
			clear_screen(chip);
			*pc += 2;
			break;
		case 0x000E: /* 00EE - RET */
			--*sp;
			*pc = stack[*sp];
			*pc += 2;
			break;
		default:
			CC_ERROR("Unknown opcode [0x0000]: 0x%X\n", opcode);
			break;
		}
		break;
	case 0x1000: /* 1NNN - JP addr */
		*pc = OPCODE_GET_NNN(opcode);
		break;
	case 0x2000: /* 2NNN - CALL addr */
		stack[*sp] = *pc;
		if (++*sp >= STACK_LEVEL) {
			CC_ERROR("Out of stack!\n");
			exit(2);
		}
		*pc = OPCODE_GET_NNN(opcode);
		break;
	case 0x3000: /* 3XNN - SE Vx, byte */
		*pc += V[OPCODE_GET_X_ID(opcode)] == OPCODE_GET_0NN(opcode) ? 4 : 2;
		break;
	case 0x4000: /* 4XNN - SNE Vx, byte */
		*pc += V[OPCODE_GET_X_ID(opcode)] != OPCODE_GET_0NN(opcode) ? 4 : 2;
		break;
	case 0x5000: /* 5XY0 - SE Vx, Vy */
		*pc += V[OPCODE_GET_X_ID(opcode)] == V[OPCODE_GET_Y_ID(opcode)] ? 4 : 2;
		break;
	case 0x6000: /* 6XNN - LD Vx, byte */
		V[OPCODE_GET_X_ID(opcode)] = OPCODE_GET_0NN(opcode);
		*pc += 2;
		break;
	case 0x7000: /* 7XNN - ADD Vx, byte */
		V[OPCODE_GET_X_ID(opcode)] += OPCODE_GET_0NN(opcode);
		*pc += 2;
		break;
	case 0x8000:
		switch (OPCODE_GET_00N(opcode)) {
		case 0x0000: /* 8XY0 - LD Vx, Vy */
			V[OPCODE_GET_X_ID(opcode)] = V[OPCODE_GET_Y_ID(opcode)];
			*pc += 2;
			break;
		case 0x0001: /* 8XY1 - OR Vx, Vy */
			V[OPCODE_GET_X_ID(opcode)] |= V[OPCODE_GET_Y_ID(opcode)];
			*pc += 2;
			break;
		case 0x0002: /* 8XY2 - AND Vx, Vy */
			V[OPCODE_GET_X_ID(opcode)] &= V[OPCODE_GET_Y_ID(opcode)];
			*pc += 2;
			break;
		case 0x0003: /* 8XY3 - XOR Vx, Vy */
			V[OPCODE_GET_X_ID(opcode)] ^= V[OPCODE_GET_Y_ID(opcode)];
			*pc += 2;
			break;
		case 0x0004: /* 8XY4 - ADD Vx, Vy */
			/* Check if there's a carry */
			V[0xF] = V[OPCODE_GET_Y_ID(opcode)] > (0xFF - V[OPCODE_GET_X_ID(opcode)]) ? 1 : 0;
			V[OPCODE_GET_X_ID(opcode)] += V[OPCODE_GET_Y_ID(opcode)];
			*pc += 2;
			break;
		case 0x0005: /* 8XY5 - SUB Vx, Vy */
			/* Check if there's a borrow */
			V[0xF] = V[OPCODE_GET_X_ID(opcode)] < V[OPCODE_GET_Y_ID(opcode)] ? 0 : 1;
			V[OPCODE_GET_X_ID(opcode)] -= V[OPCODE_GET_Y_ID(opcode)];
			*pc += 2;
			break;
		case 0x0006: /* 8XY6 - SHR Vx {, Vy} */
			V[0xF] = V[OPCODE_GET_X_ID(opcode)] & 0x1;
			V[OPCODE_GET_X_ID(opcode)] >>= 1;
			*pc += 2;
			break;
		case 0x0007: /* 8XY7 - SUBN Vx, Vy */
			/* Check if there's a borrow */
			V[0xF] = V[OPCODE_GET_X_ID(opcode)] > V[OPCODE_GET_Y_ID(opcode)] ? 0 : 1;
			V[OPCODE_GET_X_ID(opcode)] = V[OPCODE_GET_Y_ID(opcode)] - V[OPCODE_GET_X_ID(opcode)];
			*pc += 2;
			break;
		case 0x000E: /* 8XYE - SHL Vx {, Vy} */
			V[0xF] = V[OPCODE_GET_X_ID(opcode)] >> 7;
			V[OPCODE_GET_X_ID(opcode)] <<= 1;
			*pc += 2;
			break;
		default:
			CC_ERROR("Unknown opcode [0x8000]: 0x%X\n", opcode);
			break;
		}
		break;
	case 0x9000: /* 9XY0 - SNE Vx, Vy */
		*pc += V[OPCODE_GET_X_ID(opcode)] != V[OPCODE_GET_Y_ID(opcode)] ? 4 : 2;
		break;
	case 0xA000: /* ANNN - LD I, addr */
		*addr = OPCODE_GET_NNN(opcode);
		*pc += 2;
		break;
	case 0xB000: /* BNNN - JP V0, addr */
		*pc = V[0] + OPCODE_GET_NNN(opcode);
		break;
	case 0xC000: /* CXNN - RND Vx, byte */
		V[OPCODE_GET_X_ID(opcode)] = (rand() % 0xFF) & OPCODE_GET_0NN(opcode);
		*pc += 2;
		break;
	case 0XD000: /* DXYN - DRW Vx, Vy, nibble */
		draw_sprite(chip,
					V[OPCODE_GET_X_ID(opcode)],
					V[OPCODE_GET_Y_ID(opcode)],
					OPCODE_GET_00N(opcode));
		*pc += 2;
		break;
	case 0xE000:
		switch (OPCODE_GET_0NN(opcode)) {
		case 0x009E: /* EX9E - SKP Vx */
			*pc += key[OPCODE_GET_X_ID(opcode)] ? 4 : 2;
			break;
		case 0x00A1: /* EXA1 - SKNP Vx */
			*pc += !key[OPCODE_GET_X_ID(opcode)] ? 4 : 2;
			break;
		default:
			CC_ERROR("Unknown opcode [0xE000]: 0x%X\n", opcode);
			break;
		}
		break;
	case 0xF000:
		switch (OPCODE_GET_0NN(opcode)) {
		case 0x0007: /* FX07 - LD Vx, DT */
			V[OPCODE_GET_X_ID(opcode)] = *delay;
			*pc += 2;
			break;
		case 0x000A: /* FX0A - LD Vx, K */
			for (int i = 0; i < 16; i++)
				if (key[i]) {
					V[OPCODE_GET_X_ID(opcode)] = i;
					pressed = TRUE;
				}
			if (!pressed)
				return;
			*pc += 2;
			break;
		case 0x0015: /* FX15 - LD DT, Vx */
			*delay = V[OPCODE_GET_X_ID(opcode)];
			*pc += 2;
			break;
		case 0x0018: /* FX18 - LD ST, Vx */
			*sound = V[OPCODE_GET_X_ID(opcode)];
			*pc += 2;
			break;
		case 0x001E: /* FX1E - ADD I, Vx */
			/* Check if there's a overflow */
			V[0xF] = *addr + V[OPCODE_GET_X_ID(opcode)] > 0xFFF ? 1 : 0;
			*addr += V[OPCODE_GET_X_ID(opcode)];
			*pc += 2;
			break;
		case 0x0029: /* FX29 - LD F, Vx */
			*addr = V[OPCODE_GET_X_ID(opcode)] * 5;
			*pc += 2;
			break;
		case 0x0033: /* FX33 - LD B, Vx */
			memory[*addr] = V[OPCODE_GET_X_ID(opcode)] / 100;
			memory[*addr + 1] = (V[OPCODE_GET_X_ID(opcode)] / 10) % 10;
			memory[*addr + 2] = (V[OPCODE_GET_X_ID(opcode)] % 100) % 10;
			*pc += 2;
			break;
		case 0x0055: /* FX55 - LD [I], Vx */
			for (int i = 0; i < OPCODE_GET_X_ID(opcode); i++)
				memory[*addr + i] = V[i];
			/* on the original interpreter, when the operation is done I = I + X + 1 */
			*addr += OPCODE_GET_X_ID(opcode) + 1;
			*pc += 2;
			break;
		case 0x0065: /* FX65 - LD Vx, [I] */
			for (int i = 0; i < OPCODE_GET_X_ID(opcode); i++)
				V[i] = memory[*addr + i];
			/* on the original interpreter, when the operation is done I = I + X + 1 */
			*addr += OPCODE_GET_X_ID(opcode) + 1;
			*pc += 2;
			break;
		default:
			CC_ERROR("Unknown opcode [0xF000]: 0x%X\n", opcode);
			break;
		}
		break;
	default:
		CC_ERROR("Unknown opcode: 0x%X\n", opcode);
		break;
	}

	/* update timers */
	if (*delay > 0)
		--*delay;

	if (*sound > 0) {
		if (*sound == 1)
			CC_INFO("BEEP!\n");

		--*sound;
	}
}

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chip8.h"

#define OPCODE_GET_NNN(op) ((op) & 0x0FFF) /* 12-bit address */
#define OPCODE_GET_0NN(op) ((op) & 0x00FF) /* 8-bit constant */
#define OPCODE_GET_00N(op) ((op) & 0x000F) /* 4-bit constant */

#define OPCODE_GET_X_ID(op) (((op) & 0x0F00) >> 8) /* 4-bit x register identifier */
#define OPCODE_GET_Y_ID(op) (((op) & 0x00F0) >> 4) /* 4-bit y register identifier */

enum navigation {
	UP,
	DOWN,
	LEFT,
	RIGHT,
};

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

#ifdef SUPPORT_SCHIP_48
static const char schip48_digit[100] = {
	0x00, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, /* 0 */
	0x00, 0x08, 0x18, 0x28, 0x48, 0x08, 0x08, 0x08, 0x08, 0x7F, /* 1 */
	0x00, 0xFF, 0x01, 0x01, 0x01, 0xFF, 0x80, 0x80, 0x80, 0xFF, /* 2 */
	0x00, 0xFF, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x01, 0x01, 0xFF, /* 3 */
	0x00, 0x81, 0x81, 0x81, 0x81, 0xFF, 0x01, 0x01, 0x01, 0x0F, /* 4 */
	0x00, 0xFF, 0x80, 0x80, 0x80, 0xFF, 0x01, 0x01, 0x01, 0xFF, /* 5 */
	0x00, 0xFF, 0x80, 0x80, 0x80, 0xFF, 0x81, 0x81, 0x81, 0xFF, /* 6 */
	0x00, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /* 7 */
	0x00, 0xFF, 0x81, 0x81, 0x81, 0xFF, 0x81, 0x81, 0x81, 0xFF, /* 8 */
	0x00, 0xFF, 0x81, 0x81, 0x81, 0xFF, 0x01, 0x01, 0x01, 0xFF  /* 9 */
};
#endif

static inline void clear_screen(struct chip8 *chip)
{
	memset(chip->gfx, 0, SCREEN_PIXELS * sizeof(uint8_t));
#ifdef SUPPORT_SCHIP_48
	memset(chip->xgfx, 0, XSCREEN_PIXELS * sizeof(uint8_t));
#endif
	chip->draw_flag = TRUE;
}

static void draw_sprite(struct chip8 *chip, uint8_t x, uint8_t y, uint8_t h)
{
	uint8_t p;
	uint8_t *flag = chip->reg8 + 0xF;
	uint8_t *gfx = chip->gfx;
	const uint8_t *memory = chip->memory;
	const uint16_t addr = chip->reg16;

	if (h == 0) /* fallback from extend mode */
		h = 16;

	/* check position overflow */
	while (x > SCREEN_WIDTH)
		x -= SCREEN_WIDTH;
	while (y > SCREEN_HEIGHT)
		y -= SCREEN_HEIGHT;

	*flag = 0;

	for (int r = 0; r < h; r++) {
		p = memory[addr + r];
		for (int n = 0; n < 8; n++) {
			if ((p & (0x80 >> n)) != 0) {
				if (gfx[(y + r) * SCREEN_WIDTH + (x + n)] != 0)
					*flag = 1;
				gfx[(y + r) * SCREEN_WIDTH + (x + n)] ^= 1;
			}
		}
	}

	chip->draw_flag = TRUE;
}

#ifdef SUPPORT_SCHIP_48
static void draw_xsprite(struct chip8 *chip, uint8_t x, uint8_t y, uint8_t h)
{
	uint8_t p;
	uint8_t *flag = chip->reg8 + 0xF;
	uint8_t *xgfx = chip->xgfx;
	const uint8_t *memory = chip->memory;
	const uint16_t addr = chip->reg16;

	if (!chip->extend) {
		draw_sprite(chip, x, y, h);
		return;
	}

	/* check position overflow */
	while (x > XSCREEN_WIDTH)
		x -= XSCREEN_WIDTH;
	while (y > XSCREEN_HEIGHT)
		y -= XSCREEN_HEIGHT;

	*flag = 0;

	for (int r = 0; r < 16; r++) {
		p = memory[addr + r * 2];
		for (int n = 0; n < 8; n++)
			if ((p & (0x80 >> n)) != 0) {
				if (xgfx[(y + r) * XSCREEN_WIDTH + (x + n)] != 0)
					*flag = 1;
				xgfx[(y + r) * XSCREEN_WIDTH + (x + n)] ^= 1;
			}
		p = memory[addr + r * 2 + 1];
		for (int n = 0; n < 8; n++)
			if ((p & (0x80 >> n)) != 0) {
				if (xgfx[(y + r) * XSCREEN_WIDTH + (x + n)] != 0)
					*flag = 1;
				xgfx[(y + r) * XSCREEN_WIDTH + (x + n)] ^= 1;
			}
	}

	chip->draw_flag = TRUE;
}

static void scroll_display(struct chip8 *chip, uint8_t n, enum navigation nav)
{
	int i;
	uint8_t x;
	uint8_t sw = chip->extend ? XSCREEN_WIDTH : SCREEN_WIDTH;
	uint8_t sh = chip->extend ? XSCREEN_HEIGHT : SCREEN_HEIGHT;

	if (nav == RIGHT) {
		for (int y = 0; y < sh; y++) {
			x = sw - 1;
			while (x >= 0) {
				i = y * sw + x;
				if (x > 3)
					(chip->extend ? chip->xgfx : chip->gfx)[i] =
						(chip->extend ? chip->xgfx : chip->gfx)[i - 4];
				else
					(chip->extend ? chip->xgfx : chip->gfx)[i] = 0;
				x--;
			}
		}
		chip->draw_flag = TRUE;
	} else if (nav == LEFT) {
		for (int y = 0; y < sh; y++) {
			x = 0;
			while (x <= sw - 1) {
				i = y * sw + x;
				if (x < sw - 4)
					(chip->extend ? chip->xgfx : chip->gfx)[i] =
						(chip->extend ? chip->xgfx : chip->gfx)[i + 4];
				else
					(chip->extend ? chip->xgfx : chip->gfx)[i] = 0;
			}
			x++;
		}
		chip->draw_flag = TRUE;
	} else if (nav == DOWN) {
		int ni = n * (chip->extend ? XSCREEN_WIDTH : SCREEN_WIDTH);
		i = (chip->extend ? XSCREEN_PIXELS : SCREEN_PIXELS) - 1;
		while (i >= 0) {
			if (i >= ni)
				(chip->extend ? chip->xgfx : chip->gfx)[i] =
					(chip->extend ? chip->xgfx : chip->gfx)[i - ni];
			else
				(chip->extend ? chip->xgfx : chip->gfx)[i] = 0;
			i--;
		}
		chip->draw_flag = TRUE;
	} else if (nav == UP) {
		CC_INFO("Up navigation: do nothing\n");
	} else {
		CC_WARNING("Unknown navigation value: %d!\n", nav);
	}
}
#endif

void chip8_init(struct chip8 *chip)
{
	chip->pc = 0x200;    /* program counter starts at 0x200 */
	chip->opcode = 0;    /* reset opcode */
	chip->reg16 = 0;     /* reset register address */
	chip->sp = 0;        /* reset stack pointer */
	chip->compa = FALSE; /* reset compatibility */

	/* clear display */
	clear_screen(chip);
	/* clear stack */
	memset(chip->stack, 0, STACK_LEVEL * sizeof(uint16_t));
	/* clear registers V0-VF */
	memset(chip->reg8, 0, V_REGISTERS * sizeof(uint8_t));
	/* clear keypad */
	memset(chip->key, 0, KEYPAD * sizeof(uint8_t));
	/* clear memory */
	memset(chip->memory, 0, CHIP8_MEMORY_SIZE * sizeof(uint8_t));
	/* clear gfx */
	memset(chip->gfx, 0, SCREEN_PIXELS * sizeof(uint8_t));
#ifdef SUPPORT_SCHIP_48
	chip->extend = FALSE;
	/* clear RPL user flags */
	memset(chip->rpl, 0, RPL_USER_FLAGS * sizeof(uint8_t));
	/* clear xgfx */
	memset(chip->xgfx, 0, XSCREEN_PIXELS * sizeof(uint8_t));
#endif

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
		   && (total + INTERPRETER_SPACE) < CHIP8_MEMORY_SIZE) {
		memcpy(chip->memory + INTERPRETER_SPACE + total, buf, n);
		total += n;
	}

	fclose(fp);

	if ((total + INTERPRETER_SPACE) >= CHIP8_MEMORY_SIZE) {
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
	uint8_t *copa = &chip->compa;
#ifdef SUPPORT_SCHIP_48
	uint8_t *rpl = chip->rpl;
	uint8_t *extend = &chip->extend;
#endif
	const uint8_t *key = chip->key;

	const uint16_t opcode = chip->opcode = memory[*pc] << 8 | memory[*pc + 1];
	/* decodes opcodes of CHIP-8 and SCHIP-48*/
	switch (opcode & 0xF000) {
	case 0x0000:
#ifdef SUPPORT_SCHIP_48
		switch (opcode & 0x00F0) {
		case 0x00C0: /* 00CN - SCR  */
			scroll_display(chip, OPCODE_GET_00N(opcode), DOWN);
			*pc += 2;
			goto exit;
		default:
			break;
		}
#endif
		switch (OPCODE_GET_0NN(opcode)) {
		case 0x00E0: /* 00E0 - CLS */
			clear_screen(chip);
			*pc += 2;
			break;
		case 0x00EE: /* 00EE - RET */
			--*sp;
			*pc = stack[*sp];
			*pc += 2;
			break;
		case 0x00FA: /* 00FA - [EMU] turn on copatibility mode */
			*copa = TRUE;
			*pc += 2;
			break;
#ifdef SUPPORT_SCHIP_48
		case 0x00FB: /* 00FB - SCR  */
			scroll_display(chip, 4, RIGHT);
			*pc += 2;
			break;
		case 0x00FC: /* 00FC - SCL  */
			scroll_display(chip, 4, LEFT);
			*pc += 2;
			break;
		case 0x00FD: /* 00FD - EXIT  */
			exit(0);
		case 0x00FE: /* 00FE - LOW  */
			*extend = FALSE;
			clear_screen(chip);
			*pc += 2;
			break;
		case 0x00FF: /* 00FE - HIGH  */
			*extend = TRUE;
			clear_screen(chip);
			*pc += 2;
			break;
#endif
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
		if (OPCODE_GET_00N(opcode))
			draw_sprite(chip,
						V[OPCODE_GET_X_ID(opcode)],
						V[OPCODE_GET_Y_ID(opcode)],
						OPCODE_GET_00N(opcode));
#ifdef SUPPORT_SCHIP_48
		else
			draw_xsprite(chip,
						V[OPCODE_GET_X_ID(opcode)],
						V[OPCODE_GET_Y_ID(opcode)],
						0);
#endif
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
#ifdef SUPPORT_SCHIP_48
		case 0x0030: /* FX30 - LD HF, Vx  */
			*addr = V[OPCODE_GET_X_ID(opcode)] * 10;
			*pc += 2;
			break;
#endif
		case 0x0033: /* FX33 - LD B, Vx */
			memory[*addr] = V[OPCODE_GET_X_ID(opcode)] / 100;
			memory[*addr + 1] = (V[OPCODE_GET_X_ID(opcode)] / 10) % 10;
			memory[*addr + 2] = (V[OPCODE_GET_X_ID(opcode)] % 100) % 10;
			*pc += 2;
			break;
		case 0x0055: /* FX55 - LD [I], Vx */
			memcpy(memory + *addr, V, OPCODE_GET_X_ID(opcode));
			if (chip->compa)
				*addr += OPCODE_GET_X_ID(opcode) + 1;
			*pc += 2;
			break;
		case 0x0065: /* FX65 - LD Vx, [I] */
			memcpy(V, memory + *addr, OPCODE_GET_X_ID(opcode));
			if (chip->compa)
				*addr += OPCODE_GET_X_ID(opcode) + 1;
			*pc += 2;
			break;
#ifdef SUPPORT_SCHIP_48
		case 0x0075: /* FX75 - LD R, Vx  */
			memcpy(rpl, V, OPCODE_GET_X_ID(opcode) + 1);
			*pc += 2;
			break;
		case 0x0085: /* FX85 - LD Vx, R  */
			memcpy(V, rpl, OPCODE_GET_X_ID(opcode) + 1);
			*pc += 2;
			break;
#endif
		default:
			CC_ERROR("Unknown opcode [0xF000]: 0x%X\n", opcode);
			break;
		}
		break;
	default:
		CC_ERROR("Unknown opcode: 0x%X\n", opcode);
		break;
	}

#ifdef SUPPORT_SCHIP_48
exit:
#endif
	/* update timers */
	if (*delay > 0)
		--*delay;

	if (*sound > 0) {
		if (*sound == 1)
			CC_INFO("BEEP!\n");

		--*sound;
	}
}

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include "chip8.h"
#include "common.h"

#define PIXEL_SIZE 5

#define CLOCK_HZ 60
#define CLOCK_RATE_MS ((int) ((1.0 / CLOCK_HZ) * 1000 + 0.5))

#define BLACK 0
#define WHITE 255

#define SCREEN_ROWS (CHIP8_SCREEN_HEIGHT * PIXEL_SIZE)
#define SCREEN_COLS (CHIP8_SCREEN_WIDTH * PIXEL_SIZE)

static unsigned char screen[SCREEN_ROWS][SCREEN_COLS][3];

static chip8_t cc_chip8;

struct timeval clock_prev;

static int timediff_ms(struct timeval *end, struct timeval *start)
{
	return ((end->tv_sec - start->tv_sec) * 1000 + (end->tv_usec - start->tv_usec) / 1000);
}

static void gfx_setup(void)
{
	memset(screen, BLACK, sizeof(unsigned char) * SCREEN_ROWS * SCREEN_COLS * 3);
	glClear(GL_COLOR_BUFFER_BIT);
}

static int key_map(unsigned char k)
{
	switch (k) {
	case '1':
		return 0x1;
	case '2':
		return 0x2;
	case '3':
		return 0x3;
	case '4':
		return 0xc;

	case 'q':
		return 0x4;
	case 'w':
		return 0x5;
	case 'e':
		return 0x6;
	case 'r':
		return 0xd;

	case 'a':
		return 0x7;
	case 's':
		return 0x8;
	case 'd':
		return 0x9;
	case 'f':
		return 0xe;
				          
	case 'z':
		return 0xa;
	case 'x':
		return 0x0;
	case 'c':
		return 0xb;
	case 'v':
		return 0xf;
	}

	return -1;
}

static void key_press(unsigned char k, int x UNUSED, int y UNUSED)
{
	int index = key_map(k);

	if (index >= 0)
		cc_chip8.key[index] = 1;
}

static void key_release(unsigned char k, int x UNUSED, int y UNUSED)
{
	int index = key_map(k);

	if (index >= 0)
		cc_chip8.key[index] = 0;
}

static void paint_pixel(int row, int col, unsigned char color)
{
	row = SCREEN_ROWS - 1 - row;
	screen[row][col][0] = screen[row][col][1] = screen[row][col][2] = color;
}

static void paint_cell(int row, int col, unsigned char color)
{
	int pixel_row = row * PIXEL_SIZE;
	int pixel_col = col * PIXEL_SIZE;

	for (int drow = 0; drow < PIXEL_SIZE; drow++)
		for (int dcol = 0; dcol < PIXEL_SIZE; dcol++)
			paint_pixel(pixel_row + drow, pixel_col + dcol, color);
}

static void draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	for (int row = 0; row < CHIP8_SCREEN_HEIGHT; row++)
		for (int col = 0; col < CHIP8_SCREEN_WIDTH; col++)
			paint_cell(row, col, cc_chip8.gfx[GFX_INDEX(row, col)] ? WHITE : BLACK);

	glDrawPixels(SCREEN_COLS, SCREEN_ROWS, GL_RGB, GL_UNSIGNED_BYTE, (void *)screen);
	glutSwapBuffers();
}

static void loop(void)
{
	struct timeval clock_now;

	gettimeofday(&clock_now, NULL);

	chip8_emulate_cycle(&cc_chip8);

	if (cc_chip8.draw_flag) {
		draw();
		cc_chip8.draw_flag = FALSE;
	}

	if (timediff_ms(&clock_now, &clock_prev) >= CLOCK_RATE_MS) {
		chip8_tick(&cc_chip8);
		clock_prev = clock_now;
	}
}

static void reshape_window(GLsizei w UNUSED, GLsizei h UNUSED)
{
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: ./emulator rom\n");
		exit(2);
	}

	chip8_init(&cc_chip8);
	assert(chip8_load(&cc_chip8, argv[1]) == 0);

	glutInit(&argc, argv);          
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(SCREEN_COLS, SCREEN_ROWS);
	glutInitWindowPosition(150, 150);
	glutCreateWindow("CC CHIP8");

	glutDisplayFunc(draw);
	glutIdleFunc(loop);
	glutReshapeFunc(reshape_window);

	glutKeyboardFunc(key_press);
	glutKeyboardUpFunc(key_release);

	gfx_setup();

	gettimeofday(&clock_prev, NULL);

	glutMainLoop();

	return 0;
}

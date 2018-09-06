#include <stdio.h>
#include <stdlib.h>

#include <GL/glut.h>

#include "common.h"
#include "chip8.h"

#define EMULATOR_WIDTH 64
#define EMULATOR_HEIGHT 32
#define EMULATOR_MODIFIER 10

static int window_width = EMULATOR_WIDTH * EMULATOR_MODIFIER;
static int window_height = EMULATOR_HEIGHT * EMULATOR_MODIFIER;

static struct chip8 chip;

static void draw_pixel(int x, int y)
{
	glBegin(GL_QUADS);
	glVertex3f(x * EMULATOR_MODIFIER + 0.0f, y * EMULATOR_MODIFIER + 0.0f, 0.0f);
	glVertex3f(x * EMULATOR_MODIFIER + 0.0f, y * EMULATOR_MODIFIER + EMULATOR_MODIFIER, 0.0f);
	glVertex3f(x * EMULATOR_MODIFIER + EMULATOR_MODIFIER, y * EMULATOR_MODIFIER + EMULATOR_MODIFIER, 0.0f);
	glVertex3f(x * EMULATOR_MODIFIER + EMULATOR_MODIFIER, y * EMULATOR_MODIFIER + 0.0f, 0.0f);
	glEnd();
}

static void update_quads(void)
{
	for (int y = 0; y < 32; ++y)
		for (int x = 0; x < 64; ++x) {
			if (chip.gfx[y * 64 + x] == 0)
				glColor3f(0.0f, 0.0f, 0.0f);
			else
				glColor3f(1.0f, 1.0f, 1.0f);

			draw_pixel(x, y);
		}
}

static void emulator_reshape(GLsizei w, GLsizei h)
{
	glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);

	/* Resize quad */
	window_width = w;
	window_height = h;
}

static void emulator_display(void)
{
	chip8_emulate_cycle(&chip);

	if (chip.draw_flag) {
		glClear(GL_COLOR_BUFFER_BIT);
		update_quads();
		glutSwapBuffers();
		chip.draw_flag = FALSE;
	}
}

static void emulator_kb_down(uint8_t key, int x UNUSED, int y UNUSED)
{
	if (key == 27) /* esc */
		exit(0);

	if (key == '1')
		chip.key[0x1] = 1;
	else if(key == '2')
		chip.key[0x2] = 1;
	else if(key == '3')
		chip.key[0x3] = 1;
	else if(key == '4')
		chip.key[0xC] = 1;
	else if(key == 'q')
		chip.key[0x4] = 1;
	else if(key == 'w')
		chip.key[0x5] = 1;
	else if(key == 'e')
		chip.key[0x6] = 1;
	else if(key == 'r')
		chip.key[0xD] = 1;
	else if(key == 'a')
		chip.key[0x7] = 1;
	else if(key == 's')
		chip.key[0x8] = 1;
	else if(key == 'd')
		chip.key[0x9] = 1;
	else if(key == 'f')
		chip.key[0xE] = 1;
	else if(key == 'z')
		chip.key[0xA] = 1;
	else if(key == 'x')
		chip.key[0x0] = 1;
	else if(key == 'c')
		chip.key[0xB] = 1;
	else if(key == 'v')
		chip.key[0xF] = 1;

	CC_INFO("Key pressed: %c\n", key);
}

static void emulator_kb_up(uint8_t key, int x UNUSED, int y UNUSED)
{
	if (key == '1')
		chip.key[0x1] = 0;
	else if(key == '2')
		chip.key[0x2] = 0;
	else if(key == '3')
		chip.key[0x3] = 0;
	else if(key == '4')
		chip.key[0xC] = 0;
	else if(key == 'q')
		chip.key[0x4] = 0;
	else if(key == 'w')
		chip.key[0x5] = 0;
	else if(key == 'e')
		chip.key[0x6] = 0;
	else if(key == 'r')
		chip.key[0xD] = 0;
	else if(key == 'a')
		chip.key[0x7] = 0;
	else if(key == 's')
		chip.key[0x8] = 0;
	else if(key == 'd')
		chip.key[0x9] = 0;
	else if(key == 'f')
		chip.key[0xE] = 0;
	else if(key == 'z')
		chip.key[0xA] = 0;
	else if(key == 'x')
		chip.key[0x0] = 0;
	else if(key == 'c')
		chip.key[0xB] = 0;
	else if(key == 'v')
		chip.key[0xF] = 0;

	CC_INFO("Key released: %c\n", key);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s chip8_rom\n", argv[0]);
		exit(1);
	}

	chip8_init(&chip);
	chip8_load(&chip, argv[1]);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(window_width, window_height);
	glutInitWindowPosition(320, 320);
	glutCreateWindow("CHIP 8 emulator");

	glutDisplayFunc(emulator_display);
	glutIdleFunc(emulator_display);
	glutReshapeFunc(emulator_reshape);
	glutKeyboardFunc(emulator_kb_down);
	glutKeyboardFunc(emulator_kb_up);

	glutMainLoop();

	return 0;
}

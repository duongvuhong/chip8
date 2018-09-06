objects = chip8.o emulator.o

CFLAGS += -std=gnu99 -Wall -O2 -I..

LDFLAGS := -lglut -lGLU -lGL

EMULATOR = emulator

all: $(EMULATOR)

chip8.o: ../chip8.c
	$(CC) $(CFLAGS) -c $^

$(EMULATOR): $(objects)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	-@$(RM) $(EMULATOR) $(DEBUGGER) *.o *.d

.PHONY: all clean

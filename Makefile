objects = chip8.o emulator.o

CFLAGS := -std=gnu99 -Wall -DCC_VERBOSE

LDFLAGS := -lglut -lGLU -lGL

EMULATOR = emulator

all: $(EMULATOR)

$(EMULATOR): $(objects)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	-@$(RM) $(EMULATOR) $(DEBUGGER) *.o *.d

.PHONY: all clean

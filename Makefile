CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
LIBS = -lm

OBJS = main.o engine.o gas.o fb.o draw_eng.o audio.o input.o log.o

cEng: $(OBJS)
	$(CC) $(CFLAGS) -o cEng $(OBJS) $(LIBS)

clean:
	rm -f cEng $(OBJS)

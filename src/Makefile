CC      = g++
CFLAGS  = -Wall -g -Wextra -Wcast-qual -Wshadow -ansi -pedantic -std=c++11 -O3
OBJS    = main.o farm.o gen_artifact.o text_io.o types.o
EXE     = sim

all: sim

release: $(OBJS)
	$(CC) -O3 -o $(EXE) $^ -static

sim: $(OBJS)
	$(CC) -O3 -o $(EXE) $^

%.o: %.cpp
	$(CC) -c $(CFLAGS) -x c++ $< -o $@

clean:
	rm -f *.o $(EXE).exe $(EXE)
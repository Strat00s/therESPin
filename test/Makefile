CC = g++
CFLAGS = -g -O2 -Wall -Wextra -lm

all_: clean sine_gen

sine_gen:
	$(CC) $(CFLAGS) -o sine_gen sine_gen.cpp

run: sine_gen
	./sine_gen

clean:
	rm -f sine_gen *.o
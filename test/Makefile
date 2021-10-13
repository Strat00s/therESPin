CC = g++
CFLAGS = -g -O2 -Wall -Wextra -lm

all_: clean wave_gen smooth_gen

wave_gen:
	$(CC) $(CFLAGS) -o wave_gen wave_gen.cpp
smooth_gen: clean
	$(CC) $(CFLAGS) -o smooth_gen smooth_gen.cpp

clean:
	rm -f sine_gen smooth_gen *.o
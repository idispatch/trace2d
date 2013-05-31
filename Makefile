all: trace2d

trace2d: trace2d.o
	$(CC) -o $@ $^

trace2d.o: main.c
	$(CC) -g -c $^ -o $@

clean:
	rm -f trace2d


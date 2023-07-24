CC = gcc
OBJECTS = compute.o input.o main.o
SOURCES = compute.c input.c main.c
HEADERS = compute.h input.h main.h
power: $(OBJECTS)
	$(CC) $(OBJECTS) -o power -lm
main.o: main.c $(HEADERS)
compute.o: compute.c compute.h
input.o: input.c input.h
clean:
	rm *.o power

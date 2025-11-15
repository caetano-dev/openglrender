CC = gcc

TARGET = main

SRCS = main.c

CFLAGS = -g -Wall

LIBS = -lglut -lGLU -lGL -lm

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
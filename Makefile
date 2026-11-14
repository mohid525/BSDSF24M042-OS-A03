CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

TARGET = bin/myshell
SOURCES = src/main.c src/shell.c src/execute.c
OBJECTS = $(SOURCES:src/%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ -lreadline

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin build

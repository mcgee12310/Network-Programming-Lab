# Define the C compiler to use
CC = gcc

# Define compiler flags (e.g., warnings, optimizations)
CFLAGS = -Wall -Wextra -std=c11 -g

# Define the name of the executable
TARGET = main

# Define source files
SRCS = main.c

# Automatically generate object file names from source files
OBJS = $(SRCS:.c=.o)

# Default target: builds the executable
all: $(TARGET)

# Rule to link object files into the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Rule to compile individual C source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony targets (targets that don't correspond to actual files)
.PHONY: all clean

# Clean target: removes generated files
clean:
ifeq ($(OS),Windows_NT)
	del /Q $(OBJS) $(TARGET) 2>nul || true
else
	rm -f $(OBJS) $(TARGET)
endif
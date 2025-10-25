# Define the C compiler to use
CC = gcc

# Define compiler flags (e.g., warnings, optimizations)
CFLAGS = -Wall -g

# Define the name of the executable
TARGET = resolver

# Define source files
SRCS = main.c

# Default target: builds the executable
all: $(TARGET)

# Rule to link object files into the executable
$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

# Clean target: removes generated files
clean:
	rm -f $(TARGET)

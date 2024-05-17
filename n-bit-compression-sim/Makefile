# Define variables
CC = gcc
LDFLAGS = -lz
TARGET = n_bit_compression_sim
SRC = n_bit_compression_sim.c

# Default target
all: $(TARGET)

# Target to create the output directory if it doesn't exist and compile the program
$(TARGET): $(SRC) | run
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run:
	./$(TARGET)


# Clean up generated files
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean


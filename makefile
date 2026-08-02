# define compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra

# target name
TARGET = course-encoder.exe

# define source files
SRCS = main.c
OBJS = $(SRCS:.c=.o)

# default target builds everything
all: $(TARGET)

# rule to link object files
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# pattern rule to compile stuff
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# not actual files
.PHONY: all clean run

# rule to compile
run: $(TARGET)
	./$(TARGET)

# rule to clean
clean:
	rm -f $(TARGET) $(OBJS)

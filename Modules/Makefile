CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fPIC -O2
INCLUDES = -I../src -I../gc -I../rc
LIBS = -lcurl -ljson-c

# Source files
SOURCES = string_mod.c random_mod.c atomics_mod.c file_mod.c json_mod.c time_mod.c http_mod.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(OBJECTS)

# Individual object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Specific module targets
string_mod.o: string_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

random_mod.o: random_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

atomics_mod.o: atomics_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

file_mod.o: file_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

json_mod.o: json_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -c $< -o $@

time_mod.o: time_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

http_mod.o: http_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -c $< -o $@

# Test targets
test: test_modules
	./test_modules

test_modules: test_modules.c $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

# Clean
clean:
	rm -f $(OBJECTS) test_modules

.PHONY: all test clean
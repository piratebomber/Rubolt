/*
 * rubolt_io.h - I/O Library Header for Rubolt
 * File and console I/O operations
 */

#ifndef RUBOLT_IO_H
#define RUBOLT_IO_H

#include <stddef.h>

// File operations
int rb_file_open(const char* path, const char* mode);
int rb_file_close(int fd);
int rb_file_read(int fd, char* buffer, size_t size);
int rb_file_write(int fd, const char* data, size_t size);
int rb_file_exists(const char* path);
int rb_file_delete(const char* path);

// Directory operations
int rb_dir_create(const char* path);
int rb_dir_remove(const char* path);
int rb_dir_exists(const char* path);

// Console I/O
void rb_print(const char* message);
void rb_println(const char* message);
char* rb_input(const char* prompt);

#endif // RUBOLT_IO_H

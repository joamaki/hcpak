/*
 * bitfile.h - bitwise access to files
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#ifndef __BITFILE_H
#define __BITFILE_H

struct bitfile;

/* Open a bitfile from filename */
struct bitfile * bitfile_open(char *filename, const char *mode);

/* Open a bitfile from already opened file */
struct bitfile * bitfile_from_file(FILE *file, const char *mode);

/* Close a bitfile */
void bitfile_close(struct bitfile *bf);

/* Rewind a bitfile to start. Only works when opened for reading */
void bitfile_rewind(struct bitfile *bf);

/* Write things into the file (failure -> call error()) */
void bitfile_put_byte(struct bitfile *bf, u8 byte);
void bitfile_put_bytes(struct bitfile *bf, u8 *bytes, size_t count);
void bitfile_put_bit(struct bitfile *bf, u8 bit);
void bitfile_put_bits(struct bitfile *bf, u8 *bits, size_t count);
void bitfile_put_u32(struct bitfile *bf, u32 data);

/* Read things from the file. Returns non-zero if EOF. */
int bitfile_get_byte(struct bitfile *bf, u8 *res);
int bitfile_get_bytes(struct bitfile *bf, u8 *res, size_t count);
int bitfile_get_bit(struct bitfile *bf, u8 *res);
int bitfile_get_u32(struct bitfile *bf, u32 *res);

#endif /* __BITFILE_H */

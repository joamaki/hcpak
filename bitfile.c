/*
 * bitfile.c - bitwise access to files
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "util.h"
#include "bitfile.h"

/* Number of bytes to buffer at a time */
#define BITFILE_BUFFER_LEN 4096

struct bitfile {
	FILE *file;

	int big_endian;    /* non-zero if platform is big-endian */

	u8 *buffer;        /* Data buffer.
			    * If reading this grows with the input,
			    * If writing the buffer doesn't grow
			    * and gets emptied when full
			    */

	u8 *buffer_end;    /* End of buffer (allocation) */

	u8 *pos;           /* Current position in buffer */
	int bit_pos;       /* Bit-position on current byte */

	u8 *read_end;      /* Only used when reading;
			    * This signifies the end of
			    * readable part of buffer. */

	char mode;         /* Mode: 'r' (read) or 'w' (write) */
};

/* Extends buffer with more data */
static void read_buffer(struct bitfile *bf)
{
	size_t rlen;

	if (bf->buffer_end < (bf->read_end + BITFILE_BUFFER_LEN)) {
		size_t old_len = bf->buffer_end - bf->buffer;
		size_t old_pos = bf->pos - bf->buffer;
		size_t old_rend = bf->read_end - bf->buffer;

		bf->buffer = xrealloc(bf->buffer, old_len * 2);
		bf->buffer_end = bf->buffer + old_len * 2;
		bf->pos = bf->buffer + old_pos;
		bf->read_end = bf->buffer + old_rend;
	}

	rlen = fread(bf->read_end, 1, BITFILE_BUFFER_LEN, bf->file);
	if (rlen != 0) {
		bf->read_end += rlen;
	}
}

/* Writes the buffer to a file and resets position */
static void write_buffer(struct bitfile *bf)
{
	size_t wlen;

	assert(bf->mode == 'w');

	if (bf->bit_pos != 0) bf->pos++;

	assert (bf->pos <= bf->buffer_end+1);

	wlen = bf->pos - bf->buffer;
	if (wlen) {
		if (1 != fwrite(bf->buffer, wlen, 1, bf->file)) {
			error("Unable to write %d bytes "
			      "from buffer to file: %s",
			      wlen,
			      strerror(errno));
		}
	}

	memset(bf->buffer, 0, BITFILE_BUFFER_LEN);
	bf->pos = bf->buffer;
}

struct bitfile * bitfile_from_file(FILE *file, const char *mode)
{
	struct bitfile * bf = xmalloc(sizeof(struct bitfile));
	int test = 1;

	bf->file = file;

	/* Check endianess by checking if LSB is first */
	if (*((char *)&test))
		bf->big_endian = 0;
	else
		bf->big_endian = 1;

	bf->buffer = xmalloc(BITFILE_BUFFER_LEN);
	bf->buffer_end = bf->buffer + BITFILE_BUFFER_LEN;
	memset(bf->buffer, 0, BITFILE_BUFFER_LEN);

	bf->pos = bf->buffer;
	bf->read_end = bf->buffer;
	bf->bit_pos = 0;

	if (*mode == 'r')
		read_buffer(bf);

	assert (*mode == 'r' || *mode == 'w');

	bf->mode = *mode;

	return bf;
}

struct bitfile * bitfile_open(char *filename, const char *mode)
{
	FILE *file = fopen(filename, mode);
	if (file == NULL) {
		error("Unable to open (mode: %s) file %s: %s",
		      mode, filename, strerror(errno));
	}

	return bitfile_from_file(file, mode);
}

void bitfile_close(struct bitfile *bf)
{
	if (bf->mode == 'w') write_buffer(bf);

	fclose(bf->file);
	xfree(bf->buffer);
	xfree(bf);
}

void bitfile_put_byte(struct bitfile *bf, u8 byte)
{
	assert(bf->mode == 'w');

	if (bf->bit_pos == 0) {
		*bf->pos++ = byte;
		if (bf->pos >= bf->buffer_end) {
			write_buffer(bf);
		}
	} else {
		int left = bf->bit_pos;

		*bf->pos |= byte >> left;
		if (bf->pos+1 >= bf->buffer_end)
			write_buffer(bf);
		else
			bf->pos++;

		*bf->pos = byte << (8-left);
		bf->bit_pos = left;
	}
}

void bitfile_put_bytes(struct bitfile *bf, u8 *bytes, size_t count)
{
	assert(bf->mode == 'w');

	while (count > 0) {
		bitfile_put_byte(bf, *bytes++);
		count--;
	}
}

void bitfile_put_bits(struct bitfile *bf, u8 *bits, size_t count)
{
	u8 bit_pos = 0;
	assert(bf->mode == 'w');

	while (count >= 8) {
		bitfile_put_byte(bf, *bits++);
		count -= 8;
	}

	while (count > 0) {
		bitfile_put_bit(bf, BIT_GET(*bits, bit_pos));
		bit_pos++;
		count--;
	}
}

void bitfile_put_bit(struct bitfile *bf, u8 bit)
{
	assert(bf->mode == 'w');
	assert(bit == 0 || bit == 1);

	if (bit)
		BIT_SET(*bf->pos, bf->bit_pos);
	else
		BIT_UNSET(*bf->pos, bf->bit_pos);

	bf->bit_pos++;
	if (bf->bit_pos > 7) {
		bf->bit_pos = 0;
		bf->pos++;
		if (bf->pos >= bf->buffer_end) {
			write_buffer(bf);
		}
	}
}

int bitfile_get_bytes(struct bitfile *bf, u8 *res, size_t count)
{
	assert(bf->mode == 'r');

	while (count > 0) {
		if (bitfile_get_byte(bf, res++) != 0) return -1;
		count--;
	}
	return 0;
}

int bitfile_get_byte(struct bitfile *bf, u8 *res)
{
	assert(bf->mode == 'r');

	if (bf->pos >= bf->read_end) return -1;

	if (bf->bit_pos == 0) {
		*res = *bf->pos++;
		if (bf->pos >= bf->read_end)
			read_buffer(bf);
	} else {
		int left = bf->bit_pos;
		*res = *bf->pos++ << left;

		if (bf->pos >= bf->read_end)
			read_buffer(bf);

		/* Did we get the rest of the byte? */
		if (bf->pos >= bf->read_end) return -1;

		*res |= *bf->pos >> left;
	}
	return 0;
}

int bitfile_get_bit(struct bitfile *bf, u8 *res)
{
	assert(bf->mode == 'r');

	if (bf->pos >= bf->read_end) return -1;

	*res = (*bf->pos & (1<<(7-bf->bit_pos++))) != 0;
	if (bf->bit_pos > 7) {
		bf->bit_pos = 0;
		bf->pos++;
		if (bf->pos >= bf->read_end) {
			read_buffer(bf);
		}
	}
	return 0;
}

void bitfile_put_u32(struct bitfile *bf, u32 data)
{
	union {
		u32 _data;
		u8 bytes[4];
	} u;
	u._data = data;

	assert(bf->mode == 'w');

	if (bf->big_endian) {
		bitfile_put_byte(bf, u.bytes[0]);
		bitfile_put_byte(bf, u.bytes[1]);
		bitfile_put_byte(bf, u.bytes[2]);
		bitfile_put_byte(bf, u.bytes[3]);
	} else {
		bitfile_put_byte(bf, u.bytes[3]);
		bitfile_put_byte(bf, u.bytes[2]);
		bitfile_put_byte(bf, u.bytes[1]);
		bitfile_put_byte(bf, u.bytes[0]);
	}
}

int bitfile_get_u32(struct bitfile *bf, u32 *res)
{
	union {
		u32 _data;
		u8 bytes[4];
	} u;
	assert(bf->mode == 'r');

	if (bf->big_endian) {
		if (bitfile_get_byte(bf, &u.bytes[0]) != 0 ||
		    bitfile_get_byte(bf, &u.bytes[1]) != 0 ||
		    bitfile_get_byte(bf, &u.bytes[2]) != 0 ||
		    bitfile_get_byte(bf, &u.bytes[3]) != 0)
			return -1;
	} else {
		if (bitfile_get_byte(bf, &u.bytes[3]) != 0 ||
		    bitfile_get_byte(bf, &u.bytes[2]) != 0 ||
		    bitfile_get_byte(bf, &u.bytes[1]) != 0 ||
		    bitfile_get_byte(bf, &u.bytes[0]) != 0)
			return -1;
	}

	*res = u._data;

	return 0;
}

void bitfile_rewind(struct bitfile *bf)
{
	assert(bf->mode == 'r');

	bf->bit_pos = 0;
	bf->pos = bf->buffer;
}

/*
 * util.h - utility functions
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#ifndef __UTIL_H
#define __UTIL_H

/* Short versions of commonly used but long data types */
typedef unsigned char u8;
typedef unsigned int u32;

/* Allocate 'size' bytes of memory, abort on failure */
void * xmalloc(size_t size);

/* Free memory allocated with xmalloc */
void xfree(void *ptr);

/* Reallocate memory, abort on failure */
void * xrealloc(void *ptr, size_t size);

/* Print error message and abort */
void error(const char *format, ...);

/* Macros for setting and getting bits */
#define BIT_SET(byte, pos) do { (byte) |= 1 << (7-pos); } while(0)
#define BIT_UNSET(byte, pos) do { (byte) &= ~(1 << (7-pos)); } while(0)
#define BIT_GET(byte, pos) (!!((byte) & (1 << (7-pos))))

#endif /* __UTIL_H */

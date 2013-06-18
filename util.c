/*
 * util.c - utility functions
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "util.h"

void * xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (NULL == ptr) {
		perror("xmalloc");
		abort();
	}

	return ptr;
}

void * xrealloc(void *ptr, size_t size)
{
	void *newptr = realloc(ptr, size);
	if (NULL == newptr) {
		perror("xrealloc");
		abort();
	}
	return newptr;
}

void xfree(void *ptr)
{
	if (NULL == ptr) return;
	free(ptr);
}

void error(const char *format, ...)
{
	va_list ap;

	printf("Error: ");
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	printf("\n");
	exit(1);
}

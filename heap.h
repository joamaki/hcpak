/*
 * heap.h - binary heap for use with huffman's algorithm
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#ifndef __HEAP_H
#define __HEAP_H

struct heap;
struct hcnode;

/* Build heap from an array of nodes */
struct heap * heap_build(struct hcnode *nodes[], size_t count);

/* Extract smallest node from heap */
struct hcnode * heap_extract_min(struct heap *h);

/* Free an _empty_ heap */
void heap_free(struct heap *h);

/* Add a new node to the heap */
void heap_insert(struct heap *h, struct hcnode *node);

#endif /* __HEAP_H */

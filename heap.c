/*
 * heap.c - binary heap for use with huffman's algorithm
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>

#include "util.h"
#include "huffman.h"
#include "heap.h"

#define HPARENT(i) ((i-1)/2)
#define HLEFT(i) (i*2+1)
#define HRIGHT(i) (i*2+2)

struct heap {
	struct hcnode **nodes;
	size_t count;
	size_t max_count;
};

static void heapify(struct heap *heap, size_t idx)
{
	size_t smallest = 0;

	assert (idx < heap->count);

	while (1) {
		size_t l = HLEFT(idx);
		size_t r = HRIGHT(idx);

		if (l < heap->count && heap->nodes[l]->frequency < heap->nodes[idx]->frequency)
			smallest = l;
		else
			smallest = idx;

		if (r < heap->count && heap->nodes[r]->frequency < heap->nodes[smallest]->frequency)
			smallest = r;

		if (smallest != idx) {
			struct hcnode *t = heap->nodes[idx];
			heap->nodes[idx] = heap->nodes[smallest];
			heap->nodes[smallest] = t;
			idx = smallest;
		} else {
			break;
		}
	}
}

struct heap * heap_build(struct hcnode *nodes[], size_t count)
{
	int i;
	struct heap *h = xmalloc(sizeof(struct heap));
	h->nodes = xmalloc(sizeof(struct hcnode *) * count);
	h->count = count;
	h->max_count = count;
	memcpy(h->nodes, nodes, count*(sizeof(struct hcnode *)));

	for (i=count/2; i>0; i--)
		heapify(h, i);

	return h;
}

struct hcnode * heap_extract_min(struct heap *heap)
{
	struct hcnode *n;
	assert (heap != NULL);
	if (heap->count < 1) {
		error("Heap underflow!");
	}

	n = heap->nodes[0];
	heap->count--;
	heap->nodes[0] = heap->nodes[heap->count];

	if (heap->count)
		heapify(heap, 0);

	return n;
}

void heap_insert(struct heap *heap, struct hcnode *node)
{
	if (heap->max_count == heap->count) {
		/* Double the heap size */
		heap->max_count = heap->max_count * 2;
		heap->nodes = xrealloc(heap->nodes, heap->max_count);
	}
	heap->count++;
	heap->nodes[heap->count-1] = node;
	heapify(heap, heap->count-1);
}

void heap_free(struct heap *heap)
{
	/* Heap must be empty, othewise we're leaking memory */
	assert (heap->count == 0);
	xfree(heap->nodes);
	xfree(heap);
}

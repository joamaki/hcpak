/*
 * huffman.c - huffman's algorithm
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <assert.h>
#include "util.h"
#include "heap.h"
#include "huffman.h"

void huffman_make_codes(struct hcnode *root)
{
/* Macros for pushing the bitvector position forwards and backwards */
#define BITS_FWD()             \
	bit_pos++;             \
	code_len++;            \
	if (bit_pos > 7) {     \
		pos++;         \
		bit_pos = 0;   \
	}

#define BITS_BACK()            \
	code_len--;            \
	if (bit_pos < 1) {     \
		pos--;         \
		bit_pos = 7;   \
	} else {               \
		bit_pos--;     \
	}

	struct hcnode *n = root;
	u8 code[32] = {0,};
	int code_len = 0;
	u8 *pos = code;
	u8 bit_pos = 0;

	while (1) {
		while (n->left != NULL) {
			/* going left (0), unset current bit */
			BIT_UNSET(*pos, bit_pos);
			BITS_FWD();
			n = n->left;
		}

		/* Process the leaf node */
		if (n->left == NULL && n->right == NULL) {
			memcpy(n->code, code, 32);
			n->code_len = code_len;

			/* go back one level */
			BITS_BACK();
		}

		while (n->parent != NULL) {
			if (n != n->parent->right) {
				/* going right (1), set current bit */
				BIT_SET(*pos, bit_pos);
				BITS_FWD();
				n = n->parent->right;
				break;
			} else {
				/* going up */
				BITS_BACK();
				n = n->parent;
			}
		}

		if (n->parent == NULL)
			break;
	}

#undef BITS_FWD
#undef BITS_BACK
}

struct hcnode ** huffman_init(u32 freqs[], int chars[], size_t count)
{
	int i;
	struct hcnode **nodes = xmalloc(count*sizeof(struct hcnode *));
	for (i=0; i<count; i++) {
		nodes[i] = xmalloc(sizeof(struct hcnode));
		nodes[i]->left = nodes[i]->right = nodes[i]->parent = NULL;
		nodes[i]->frequency = freqs[i];
		nodes[i]->character = chars[i];
		nodes[i]->code_len = 0;
	}
	return nodes;
}

static void free_nodes(struct hcnode *node)
{
	struct hcnode *l, *r;

	l = node->left;
	r = node->right;
	xfree(node);

	if (l) free_nodes(l);
	if (r) free_nodes(r);
}

void huffman_deinit(struct hcnode **nodes, struct hcnode *root)
{
	free_nodes(root);
	xfree(nodes);
}

struct hcnode * huffman(struct hcnode **nodes, size_t count)
{
	struct heap *heap;
	struct hcnode *z, *x, *y;
	struct hcnode *root;
	int i;

	heap = heap_build(nodes, count);
	for(i=0; i<count-1; i++) {
		z = xmalloc(sizeof(struct hcnode));
		x = z->left = heap_extract_min(heap);
		y = z->right = heap_extract_min(heap);
		z->frequency = x->frequency + y->frequency;
		x->parent = y->parent = z;
		z->parent = NULL;
		heap_insert(heap, z);
	}

	root = heap_extract_min(heap);
	heap_free(heap);
	return root;
}

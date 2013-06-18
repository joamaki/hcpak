/*
 * huffman.h - huffman's algorithm
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#ifndef __HUFFMAN_H
#define __HUFFMAN_H

struct hcnode {
	/* Tree pointers */
	struct hcnode *left, *right;
	struct hcnode *parent;

	u32 frequency;
	int character; /* Only leaves have this */

	/* 32 bytes is enough, the tree has at maximum 257 leaves
	   (256 chars + EOF) so the code length (path through the tree) can
	   be then at maximum 256 bits since the tree is full:

  	    O
	   / \
	  O   O    This presents the full tree of maximum depth:
	     / \    4 leaves and the longest path is (LEAVES-1) 3.
	    O   O
	       / \
	      O   O
	*/

	u8 code[32];
	int code_len; /* Code length in bits */
};

/* Initializes 'count' one node trees, one for each character */
struct hcnode ** huffman_init(u32 freqs[], int chars[], size_t count);

/* Frees allocated resources */
void huffman_deinit(struct hcnode **nodes, struct hcnode *root);

/* The huffman's algorithm. Returns pointer to the root of the tree */
struct hcnode * huffman(struct hcnode **nodes, size_t count);

/* Generate prefix codes by traversing the tree */
void huffman_make_codes(struct hcnode *root);

#endif

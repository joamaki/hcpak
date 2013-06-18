/*
 * hcpak.c - huffman's code file compressor
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 *
 * HCPAK is a simple file compressor that uses the Huffman's algorithm.
 * (See http://en.wikipedia.org/wiki/Huffman_coding for more information)
 *
 * Compression and decompression are done in place in the style of compress,
 * gzip etc.
 *
 * == Example usage ==
 * Compressing a file:
 * $ hcpak myfile
 *
 * Decompressing a file:
 * $ hcpak -d myfile.hc
 *
 * == File format ==
 * - Magic (5 bytes): HCPAK
 * - Frequency/Character table len: 8-bit integer. This is 1 less then true
 *                                  length so it can be stored in 8-bit integer.
 *                                  This also means that we cannot compress empty
 *                                  files.
 * - Frequency/Character table: [{char, freq}, ...]; each entry is 1+32 bytes,
 *                              full alphabet being 256*33 = 8448 bytes.
 * - Data ...
 * - EOF marked by EOFCHAR (code depends on Huffman's)
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "util.h"
#include "huffman.h"
#include "bitfile.h"

/* Files */
static struct bitfile *filein = NULL;
static char *filein_name = NULL;
static struct bitfile *fileout = NULL;
static char *fileout_name = NULL;

/* Flags */
static int verbose = 0;
static int decompression = 0;
static int force = 0;

/* File magic */
#define MAGIC_LEN 5
static const u8 *magic = (u8*) "HCPAK";

/* Maximum character count */
#define MAX_CHARS 257

/* Character for representing EOF for the file */
#define EOFCHAR (MAX_CHARS)

/* Header size */
#define HEADER_LEN (MAGIC_LEN+1)

static void usage(const char *prog)
{
	printf("Compress or decompress files using Huffman's algorithm.\n");
	printf("Usage: %s [OPTIONS] INPUTFILE\n", prog);
	printf("Options:\n");
	printf("\t-d\t\tDecompress input file\n");
	printf("\t-h\t\tPrint this help\n");
	printf("\t-v\t\tVerbose mode\n");
	printf("\nProgram defaults to compression. "
	       "Compression and decompression are done in-place.\n");
	exit(0);
}

static void parse_args(int argc, char **argv)
{
	int idx = 1;
	if (argc < 2) {
		usage(argv[0]);
		return;
	}
	argc--;
	while (argc > 0) {
		if (argv[idx][0] == '-') {
			int len = strlen(argv[idx])-1;
			while (len > 0) {
				switch (argv[idx][len]) {
				case 'h':
					usage(argv[0]);
					break;
				case 'v':
					verbose = 1;
					break;
				case 'f':
					force = 1;
					break;
				case 'd':
					decompression = 1;
					break;
				}
				len--;
			}
		} else {
			/* Filename */
			if (NULL == filein) {
				filein_name = argv[idx];
			} else {
				error("Input file already specified!");
			}
		}
		idx++;
		argc--;
	}

	if (NULL == filein_name)
		usage(argv[0]);

	if (decompression) {
		size_t len = strlen(filein_name);
		len = strlen(filein_name);
		if (!force && len > 3 && strncmp(filein_name + len-3, ".hc", 3))
			error("Input file has unknown suffix, refusing to decompress.");

		filein = bitfile_open(filein_name, "rb");

		fileout_name = xmalloc(len-3 + 1);
		memset(fileout_name, 0, len-3 + 1);
		strncpy(fileout_name, filein_name, len-3);
		fileout = bitfile_open(fileout_name, "wb");
	} else {
		size_t len = strlen(filein_name);
		if (!force && len > 3 && !strncmp(filein_name + len - 3, ".hc", 3))
			error("File already compressed, refusing to compress.");

		filein = bitfile_open(filein_name, "rb");

		fileout_name = xmalloc(len+3 + 1);
		memset(fileout_name, 0, len+3 + 1);
		strcpy(fileout_name, filein_name);
		strcat(fileout_name, ".hc");
		fileout = bitfile_open(fileout_name, "wb");
	}
}

static void calculate_frequencies (struct bitfile *bf, u32 *out_freqs,
				   int *out_chars, int *out_len)
{
	u8 res;
	int i;
	u32 freqs[MAX_CHARS+1] = {0,};

	while (1) {
		if (bitfile_get_byte(bf, &res) != 0) break;
		freqs[res]++;
	}

	*out_len = 0;
	for (i=0; i<MAX_CHARS+1; i++) {
		if (freqs[i] > 0) {
			out_chars[*out_len] = i;
			out_freqs[*out_len] = freqs[i];
			(*out_len)++;
		}
	}
}

static int compress(void)
{
	u32 freqs[MAX_CHARS] = {0,};
	int chars[MAX_CHARS] = {0,};
	int freqtable_len = 0;
	struct hcnode *lookup[MAX_CHARS+1];
	struct hcnode **nodes;
	struct hcnode *root;
	int i;
	u8 byte;
	double orig_len = 0, new_len = 0;

	if (verbose)
		fprintf(stderr, "Compressing '%s' ... ", filein_name);

	calculate_frequencies(filein, freqs, chars, &freqtable_len);

	if (freqtable_len < 1)
		error("Compressing empty files is not supported.");

	/* Write magic */
	bitfile_put_bytes(fileout, (u8*)magic, MAGIC_LEN);

	/* Write frequency table */
	bitfile_put_byte(fileout, freqtable_len-1);
	for(i=0; i<freqtable_len; i++) {
		bitfile_put_byte(fileout, chars[i]);
		bitfile_put_u32(fileout, freqs[i]);
	}

	freqs[freqtable_len] = 1;
	chars[freqtable_len] = EOFCHAR;
	freqtable_len++;

	nodes = huffman_init(freqs, chars, freqtable_len);
	root = huffman(nodes, freqtable_len);
	huffman_make_codes(root);

	/* Construct lookup table for quick code lookups */
	for(i=0; i<freqtable_len; i++) {
		lookup[nodes[i]->character] = nodes[i];
	}

	/* Write data */
	bitfile_rewind(filein);

	while(bitfile_get_byte(filein, &byte) == 0) {
		bitfile_put_bits(fileout, lookup[byte]->code, lookup[byte]->code_len);
		orig_len += 8;
		new_len += lookup[byte]->code_len;
	}

	/* Write pseudo-EOF marker */
	bitfile_put_bits(fileout, lookup[EOFCHAR]->code, lookup[EOFCHAR]->code_len);

	huffman_deinit(nodes, root);

	/* Remove source file */
	unlink(filein_name);

	bitfile_close(filein);
	bitfile_close(fileout);

	if (verbose)
		fprintf(stderr, "done, %.1f%%.\n", 100 * (1 - (new_len / orig_len)));

	return 0;
}

static int decompress(void)
{

	u32 freqs[MAX_CHARS];
	int chars[MAX_CHARS];
	int freqtable_len = 0;
	struct hcnode **nodes;
	struct hcnode *root;
	struct hcnode *n;
	int i;
	u8 byte;
	u8 magicbuf[MAGIC_LEN];
	double orig_len = 0, new_len = 0;

	if (verbose)
		fprintf(stderr, "Decompressing '%s' ... ", filein_name);

	/* Check magic */
	if (bitfile_get_bytes(filein, magicbuf, MAGIC_LEN) != 0)
		error("Input too short!");

	if (memcmp(magicbuf, magic, MAGIC_LEN) != 0)
		error("Magic mismatch on input!");

	/* Get frequency table length */
	if (bitfile_get_byte(filein, &byte) != 0)
		error("Input too short!");

	/* NB: Frequency table length is stored as 1 less then true length so that we
	   can store the length 256 in 8-bit integer (range 0-255). */
	freqtable_len = byte + 1;

	/* Get frequency table */
	for (i=0; i<freqtable_len; i++) {
		if (bitfile_get_byte(filein, &byte) != 0)
			error("Input too short!");

		chars[i] = byte;
		if (bitfile_get_u32(filein, &freqs[i]) != 0)
			error("Input too short!");
	}

	freqs[freqtable_len] = 1;
	chars[freqtable_len] = EOFCHAR;
	freqtable_len++;

	nodes = huffman_init(freqs, chars, freqtable_len);
	root = huffman(nodes, freqtable_len);
	n = root;

	/* Decompress input by traversing the prefix code tree until we reach a leaf */
	while(1) {
		if (bitfile_get_bit(filein, &byte) != 0)
			break;

		orig_len++;

		if (byte) n = n->right;
		else n = n->left;

		if (n == NULL)
			error("Code not found! File corrupted?");

		/* Did we reach a leaf? */
		if (!n->right && !n->left) {
			if (n->character == EOFCHAR)
				break;

			bitfile_put_byte(fileout, (u8) n->character);
			new_len+=8;
			n = root;
		}
	}

	huffman_deinit(nodes, root);

	/* Remove source file */
	unlink(filein_name);

	bitfile_close(filein);
	bitfile_close(fileout);

	if (verbose)
		fprintf(stderr, "done, %.1f%%.\n", 100 * (1 - (new_len / orig_len)));

	return 0;
}

int main(int argc, char **argv)
{
	int ret;

	parse_args(argc, argv);

	if (decompression)
		ret = decompress();
	else
		ret = compress();

	xfree(fileout_name);

	return ret;
}

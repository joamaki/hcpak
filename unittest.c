/*
 * unittest.c - unit tests for hcpak
 *
 * Copyright (C) 2008 Jussi Mäki <joamaki@gmail.com>
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "heap.h"
#include "huffman.h"
#include "bitfile.h"

static u32 bits_to_integer(u8 *bits, int bit_count)
{
        u32 out = 0;
        u8 bit_pos = 0;
        if (bit_count > 32) return -1;

        while (bit_count > 0) {
                if (bit_pos > 7) {
                        bits++;
                        bit_pos = 0;
                }

                /* set the rightmost bit */
                out |= !!(*bits & (1 << (7-bit_pos)));
                bit_pos++;
                bit_count--;

                /* shift the output left by one bit */
                if (bit_count) out = out << 1;
        }
        return out;
}

void test_heap(void)
{
	struct heap *h;
	struct hcnode a, b, c, d, e;
	struct hcnode *nodes[3];
	d.frequency = 4;
	e.frequency = 5;
	b.frequency = 2;
	c.frequency = 3;
	a.frequency = 1;
	nodes[0] = &a; nodes[1] = &b; nodes[2] = &c;
	nodes[3] = &d; nodes[4] = &e;
	h = heap_build(nodes, 5);

	assert (1 == heap_extract_min(h)->frequency);
	assert (2 == heap_extract_min(h)->frequency);
	assert (3 == heap_extract_min(h)->frequency);
	assert (4 == heap_extract_min(h)->frequency);
	assert (5 == heap_extract_min(h)->frequency);

	heap_free(h);
}

void test_huffman(void)
{
	struct hcnode *root, *n;
	/* This example is from CLRS */
	u32 freqs[] = {5, 9, 12, 13, 16, 45};
	int chars[] = {'f','e','c','b','d','a'};
	u32 expect_codes[] = {0xc, 0xd, 0x4, 0x5, 0x7, 0x0};

	int count = 6, i;
	struct hcnode **nodes;

	nodes = huffman_init(freqs, chars, 6);
	root = huffman(nodes, 6);

	huffman_make_codes(root);
	for (i=0; i<count; i++) {
		n = nodes[i];
/* 		printf("code for character %c (freq %d): %x (len %d) (expecting %x)\n", n->character, n->frequency, bits_to_integer(n->code, n->code_len), n->code_len, expect_codes[i]); */
		assert(bits_to_integer(n->code, n->code_len) == expect_codes[i]);
	}

}

void test_huffman2(void)
{
	struct hcnode *root, *n;
	unsigned int freqs[256];
	int chars[256];
	int count = 256, i;
	struct hcnode **nodes;

	for(i=0; i<count; i++) {
		freqs[i] = i;
		chars[i] = i;
	}

	nodes = huffman_init(freqs, chars, 256);
	root = huffman(nodes, 256);

	huffman_make_codes(root);
	for (i=0; i<count; i++) {
		n = nodes[i];
		printf("code for character %d: 0x%x\n", n->character, bits_to_integer(n->code, n->code_len));
	}
}

void test_bitfile(void)
{
	struct bitfile *bf;
	char buf[20] = {0,};
	u8 res;
	u32 res32;
	int ret = 0;

	bf = bitfile_open("bitfile.test", "w");
	assert (bf != NULL);

	bitfile_put_bytes(bf, (u8*)"HCPAK", 5);

	bitfile_put_byte(bf, 'a');
	bitfile_put_byte(bf, 'b');
	bitfile_put_byte(bf, 'c');
	bitfile_put_byte(bf, 'd');

	bitfile_put_bit(bf, 0);
	bitfile_put_bit(bf, 0);
	bitfile_put_bit(bf, 1);
	bitfile_put_bit(bf, 1);

	bitfile_put_byte(bf, 'e');
	bitfile_put_byte(bf, 'f');

	bitfile_put_u32(bf, (u32)123456);

	res = 0xff;
	bitfile_put_bits(bf, &res, 5);

	bitfile_put_bit(bf, 1);
	bitfile_put_bit(bf, 0);
	bitfile_put_bit(bf, 1);
	bitfile_put_bit(bf, 0);

	/* bitfile_put_byte(bf, 'f'); */

	bitfile_close(bf);

	bf = bitfile_open("bitfile.test", "r");
	assert (bf != NULL);

	bitfile_get_bytes(bf, (u8*)buf, 5);
	buf[5] = '\0';
	assert(!memcmp(buf, "HCPAK", 5));
	/* printf("HCPAK: '%s'\n", buf); */

	ret += bitfile_get_byte(bf, &res); /* printf("a: %c\n", res); */ assert(res == 'a');
	ret += bitfile_get_byte(bf, &res); /* printf("b: %c\n", res); */ assert(res == 'b');
	ret += bitfile_get_byte(bf, &res); /* printf("c: %c\n", res); */ assert(res == 'c');
	ret += bitfile_get_byte(bf, &res); /* printf("d: %c\n", res); */ assert(res == 'd');

	ret += bitfile_get_bit(bf, &res); /* printf("0: %d\n", res); */ assert(res == 0);
	ret += bitfile_get_bit(bf, &res); /* printf("0: %d\n", res); */ assert(res == 0);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);

	ret += bitfile_get_byte(bf, &res); /* printf("e: %c\n", res); */ assert(res == 'e');
	ret += bitfile_get_byte(bf, &res); /* printf("f: %c\n", res); */ assert(res == 'f');

	ret += bitfile_get_u32(bf, &res32);
	assert (res32 == 123456);

	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("0: %d\n", res); */ assert(res == 0);
	ret += bitfile_get_bit(bf, &res); /* printf("1: %d\n", res); */ assert(res == 1);
	ret += bitfile_get_bit(bf, &res); /* printf("0: %d\n", res); */ assert(res == 0);

	assert(ret == 0);

	/* try reading past the EOF */
	assert(bitfile_get_byte(bf, &res) != 0);

	/* try rewinding the file */
	bitfile_rewind(bf);

	bitfile_get_bytes(bf, (u8*)buf, 5);
	buf[5] = '\0';
	assert(!memcmp(buf, "HCPAK", 5));

/* 	printf("HCPAK: '%s'\n", buf); */

	bitfile_close(bf);


}

void test_bitfile2(void)
{
	struct bitfile *bf = bitfile_open("/dev/urandom", "r");
	int i;
	u8 res;
	int sum = 0;

	for(i=0; i<1000000; i++) {
		bitfile_get_bit(bf, &res);
		sum += res;
	}
	printf("the sum of first million bits from urandom: %d\n", sum);

	bitfile_rewind(bf);

	sum = 0;
	for(i=0; i<1000000; i++) {
		bitfile_get_bit(bf, &res);
		sum += res;
	}
	printf("(again) the sum first million bits from urandom: %d\n", sum);

	bitfile_close(bf);

}

void test_bitfile3(void)
{
	u8 byte;
	u32 word;
	struct bitfile *bf = bitfile_open("/tmp/test", "r");
	struct bitfile *bf2 = bitfile_open("/tmp/test.out", "w");
	struct bitfile *bf3 = bitfile_open("/tmp/test.out2", "w");
	struct bitfile *bf4 = bitfile_open("/tmp/test.out3", "w");
	struct bitfile *bf5 = bitfile_open("/tmp/test.out4", "w");

	while(bitfile_get_byte(bf, &byte) == 0) {
		bitfile_put_byte(bf2, byte);
	}

	bitfile_rewind(bf);

	while(bitfile_get_bit(bf, &byte) == 0) {
		bitfile_put_bit(bf3, byte);
	}

	bitfile_rewind(bf);

	while(bitfile_get_u32(bf, &word) == 0) {
		bitfile_put_u32(bf4, word);
	}

	bitfile_rewind(bf);

	{
		u8 bytes[2];
		u8 bits[4];
		while(bitfile_get_byte(bf, &bytes[0]) == 0 &&
		      bitfile_get_byte(bf, &bytes[1]) == 0 &&
		      bitfile_get_bit(bf, &bits[0]) == 0 &&
		      bitfile_get_bit(bf, &bits[1]) == 0 &&
		      bitfile_get_bit(bf, &bits[2]) == 0 &&
		      bitfile_get_bit(bf, &bits[3]) == 0) {
			u8 out = bits[0] << 7 | bits[1] << 6 |
				bits[2] << 5 | bits[3] << 4;
			u8 foo[3];
			foo[0] = bytes[0];
			foo[1] = bytes[1];
			foo[2] = out;
 			bitfile_put_bits(bf5, (u8 *)&foo, 20);

/* 			bitfile_put_bit(bf5, bits[0]); */
/* 			bitfile_put_bit(bf5, bits[1]); */
/* 			bitfile_put_bit(bf5, bits[2]); */
/* 			bitfile_put_bit(bf5, bits[3]); */

		}
	}

	bitfile_close(bf);
	bitfile_close(bf2);
	bitfile_close(bf3);
	bitfile_close(bf4);
	bitfile_close(bf5);

}

void test_bitfile4(void)
{
	u8 foo = 170;
/* 	u8 foo2; */
/* 	u8 foo3[2] = {85, 85}; */
	struct bitfile *bf = bitfile_open("/tmp/bf-test", "w");

	bitfile_put_bit(bf, 0);
	bitfile_put_bit(bf, 0);
	bitfile_put_bit(bf, 0);
	bitfile_put_bit(bf, 0);
	bitfile_put_bit(bf, 0);

	bitfile_put_byte(bf, foo);

	bitfile_put_bit(bf, 1);
	bitfile_put_bit(bf, 1);
	bitfile_put_bit(bf, 1);


/* 	bitfile_put_bit(bf, 0); */
/* 	bitfile_put_bit(bf, 1); */
/* 	bitfile_put_bit(bf, 0); */
/* 	bitfile_put_bit(bf, 1); */

/* 	bitfile_put_bit(bf, 0); */
/* 	bitfile_put_bit(bf, 1); */
/* 	bitfile_put_bit(bf, 0); */
/* 	bitfile_put_bit(bf, 1); */

/* 	bitfile_put_bit(bf, 0); */
/* 	bitfile_put_bit(bf, 0); */

/* 	foo2 = foo << 4; */
/* 	bitfile_put_bits(bf, &foo, 4); */
/* 	bitfile_put_bits(bf, &foo2, 4); */

/* 	bitfile_put_bits(bf, &foo3, 10); */
/* 	bitfile_put_bits(bf, &foo3, 6); */


	bitfile_close(bf);

}

int main(void)
{
	test_heap();
	test_huffman();
	test_bitfile();

/* These are manual tests: */
/* 	test_huffman2(); */
/* 	test_bitfile2(); */
/* 	test_bitfile3(); */
	test_bitfile4();
	printf("Tests passed.\n");
	return 0;
}

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

huffman: huffman.c
	$(CC) $(CFLAGS) -o huffman huffman.c

clean:
	rm -f huffman *.huf *.volta

crep: crep.c
	$(CC) crep.c -o crep -Wall -Wextra -pedantic -std=c99 -g -fsanitize=address

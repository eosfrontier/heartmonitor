test: MAX30102.h

test: test.c MAX30102.c
	gcc -Wno-unused -W -Wall -o $@ $^

testMAX: MAX30102.h

testMAX: testMAX.c MAX30102.c
	gcc -Wno-unused -W -Wall -o $@ $^

test: testMAX
	./testMAX

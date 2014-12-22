all:	input
	gcc -o read read.c -Wall

input:
	dd if=/dev/urandom of=input_file bs=1M count=1

clean:
	-rm input_file read

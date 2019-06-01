CC = arm-none-linux-gnueabi-gcc --static

all : main input output
main : main.o
	$(CC) -o main main.o
input : input.o
	$(CC) -o input input.o
output : output.o
	$(CC) -o output output.o

clean :
	rm *.o main input output

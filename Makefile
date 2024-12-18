FLAGS=`pkg-config --cflags --libs libdrm`
FLAGS+=-Wall -O0 -g
FLAGS+=-D_FILE_OFFSET_BITS=64

all:
	gcc -o projekt projekt.c $(FLAGS)
	#gcc -o projekt kms-app.c $(FLAGS)

ulimit -s 1318912
rm test.o
gcc allocator.c baggy_init.c test.c move_stack.S  -o test.o -Wall -g
#gcc allocator.c baggy_init.c test.c move_stack.S -O3 -o test.o -Wall -g
./test.o

ulimit -s 1056896
rm test.o
gcc allocator.c baggy_init.c test.c move_stack.S  -o test.o -Wall -g
./test.o

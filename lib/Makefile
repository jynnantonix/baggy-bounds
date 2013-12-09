all : build/baggylib.a

CC = gcc
CFLAGS = -std=gnu99 -g
ARFLAGS = -ru

CPP_FILES = initialization
C_FILES = allocator baggy_init baggy_slowpath pointer util
S_FILES = move_stack
H_FILES = address_constants.h

OBJECT_FILES = $(addprefix build/, $(addsuffix .o, $(C_FILES) $(S_FILES) $(CPP_FILES)))

build/baggylib.a : $(OBJECT_FILES)
	$(AR) $(ARFLAGS) $@ $(OBJECT_FILES)

build/move_stack.o : move_stack.S $(H_FILES)
	$(CC) $(CFLAGS) -c -o build/move_stack.o move_stack.S

build/initialization.o : initialization.cpp $(H_FILES)
	g++ -g -c -o build/initialization.o initialization.cpp

build/%.o : %.c $(H_FILES)
	$(CC) $(CFLAGS) -c -o $@ $<

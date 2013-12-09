.PRECIOUS: %.bg.ll %.ll

all: tests/test-1.out tests/test-2.out tests/test-3.out tests/test-4.out tests/test-5.out tests/test-6.out tests/test-7.out tests/test-8.out tests/test-9.out tests/test-10.out tests/test-11.out tests/test-12.out

%.bc : %.cpp
	clang++ -O2 -emit-llvm $< -c -o $@

%.bg.ll : %.ll
	opt -load ../pass/build/BaggyBounds/BaggyBounds.so -baggy-pointers -baggy-globals -baggy-save-local -S -o $@ $<

%.ll : %.bc
	llvm-dis -o $@ $<

%.o : %.ll
	llc -filetype=obj -o $@ $<

#CSUDIR = ../../glibc/build/csu
%.out : %.o ../lib/build/baggylib.a
	# mark baggy_force_initialization as undefined to force
	# the linker to pull in initialization stuff from the archive
	g++ -u baggy_force_initialization $< ../lib/build/baggylib.a -o $@

#gcc -nostartfiles $(CSUDIR)/crt1.o $(CSUDIR)/crti.o $< ../lib/build/baggylib.a $(CSUDIR)/crtn.o -o $@

clean:
	rm -rf tests/*.bc
	rm -rf tests/*.ll
	rm -rf tests/*.bg.ll
	rm -rf tests/*.o
	rm -rf tests/*.bg.o
	rm -rf tests/*.out
	rm -rf tests/*.bg.out

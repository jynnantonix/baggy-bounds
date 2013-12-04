.PRECIOUS: %.bg.ll %.ll

all: tests/test-1.out tests/test-2.out tests/test-3.out tests/test-4.out

%.bc : %.cpp
	clang++ -O2 -emit-llvm $< -c -o $@

%.ll : %.bc
	llvm-dis -o $@ $<

%.bg.ll : %.ll
	opt -load ../pass/build/BaggyBounds/BaggyBounds.so -baggy-pointers -baggy-globals -baggy-save-local -S -o $@ $<

%.bgo : %.bg.ll
	llc -filetype=obj -o $@ $<

CSUDIR = ../../glibc/build/csu
%.out : %.bgo
	#gcc -nostartfiles $(CSUDIR)/crt1.o $(CSUDIR)/crti.o $< ../lib/build/baggylib.a $(CSUDIR)/crtn.o -o $@
	gcc $< ../lib/build/baggylib.a -o $@

clean:
	rm -rf tests/*.ll
	rm -rf tests/*.bg.ll
	rm -rf tests/*.bc
	rm -rf tests/*.bgo
	rm -rf tests/*.o
	rm -rf tests/*.out

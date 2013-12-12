.PRECIOUS: %.bg.ll %.ll

all: tests/test-1.out tests/test-2.out tests/test-3.out tests/test-4.out tests/test-5.out tests/test-6.out tests/test-7.out tests/test-8.out tests/test-9.out tests/test-10.out tests/test-11.out tests/test-12.out

%.gccout : %.cpp
	g++ -O2 $< -o $@

%.clangout : %.cpp
	clang++ -O2 $< -o $@

%.bc : %.cpp
	clang++ -O2 -emit-llvm $< -c -o $@

%.bg.ll : %.ll ../pass/build/BaggyBounds/BaggyBounds.so
	opt -load ../pass/build/BaggyBounds/BaggyBounds.so -baggy-save-local -baggy-pointers -dce -constprop -baggy-globals -baggy-rewriter -S -o $@ $<

%.ll : %.bc
	llvm-dis -o $@ $<

%.o : %.ll
	llc -filetype=obj -o $@ $<

CSUDIR = ../lib/start
%.out : %.o ../lib/build/baggylib.a
	g++ -nostartfiles $(CSUDIR)/crt1.o $(CSUDIR)/crti.o $< ../lib/build/baggylib.a $(CSUDIR)/crtn.o -o $@
#g++ -u baggy_force_initialization $< ../lib/build/baggylib.a -o $@

clean:
	rm -rf tests/*.gccout
	rm -rf tests/*.bc
	rm -rf tests/*.ll
	rm -rf tests/*.bg.ll
	rm -rf tests/*.o
	rm -rf tests/*.bg.o
	rm -rf tests/*.out
	rm -rf tests/*.bg.out
	rm -rf tests/*.clangout
	rm -rf tests/*.gccout

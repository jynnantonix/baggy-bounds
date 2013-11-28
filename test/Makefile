.PRECIOUS: %.bg.ll %.ll

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
	rm -rf *.ll
	rm -rf *.bg.ll
	rm -rf *.bc
	rm -rf *.bgo
	rm -rf *.o
	rm -rf *.out

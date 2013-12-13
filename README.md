6.858-baggy
===========

6.858 final project - baggy bounds checking in LLVM

test/
	Our tests directory. Run test/runtests.sh or tests/benchmarks.sh to run tests or benchmarks.
	You will need clang and llvm installed.
	To make BaggyBounds.so you need the LLVM source but we have included the built version here.
pass/BaggyBounds/
	Code for the LLVM passes we wroe
pass/build/
	Where the passes are built
lib/
	Source code for our baggy bounds library
lib/zoobar_makefile
	Use this as the Makefile in the lab code to make the zoobar server with baggy bounds.
	After making, run ./zookld zook.conf to run. We have test this on commit
	f97f34afd4d414194bdf0964b1406cb5350b69e2 of the webserver (the initial lab1 code).

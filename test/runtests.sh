ulimit -s 1318912
make -C ../pass/build/ > /dev/null
make -C ../lib/ > /dev/null
./find_and_run_tests.py "$@"

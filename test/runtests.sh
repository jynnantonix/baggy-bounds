ulimit -s 1318912
BASEDIR=`dirname $0`
make clean > /dev/null
make -C $BASEDIR/../pass/build/ > /dev/null
make -C $BASEDIR/../lib/ > /dev/null
$BASEDIR/find_and_run_tests.py "$@"

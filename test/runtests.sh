make all
ulimit -s 1318912

for file in `dirname $0`/tests/*.out; do
	output=`dirname $0`/outputs/`basename $file`.output
	python runtest.py "$file" "$output"
	if [ "$?" != "0" ]; then
		echo "Test $file failed. :( :( :("
	else
		echo "Test $file passed!"
	fi
done

g++ performance_graph.cpp -O2 -Wall -o graph.o
./graph.o < benchoutput.txt > stats
python plot.py
rm graph.o


heat: heat.cpp
	mpic++ -std=c++17 -O3 -ffp-contract=off -Wall -o heat heat.cpp

clean:
	rm -f heat *.bin *.png
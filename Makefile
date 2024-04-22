all:
	g++ -std=c++20 -Wstring-compare main.cc -o main
debug:
	g++ -std=c++20 -Wstring-compare -DDEBUG -DINFO main.cc -o main
info:
	g++ -std=c++20 -Wstring-compare -DINFO main.cc -o main

clean:
	rm main

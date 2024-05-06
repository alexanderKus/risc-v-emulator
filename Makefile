all:
	g++ -std=c++20 -Wstring-compare main.cc -o main
debug:
	g++ -std=c++20 -Wstring-compare -DDEBUG -DINFO -DREGDUMP main.cc -o main
info:
	g++ -std=c++20 -Wstring-compare -DINFO main.cc -o main
regdump:
	g++ -std=c++20 -Wstring-compare -DREGDUMP main.cc -o main

clean:
	rm main

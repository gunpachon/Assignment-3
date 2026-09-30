CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

game: main.o game.o
	$(CXX) $(CXXFLAGS) -o game main.o game.o

main.o: main.cpp game.h
game.o: game.cpp game.h

clean:
	rm -f game *.o

CC=g++ -std=c++17 -Wall -Wno-deprecated
CFLAGS=-O
EXEC=shadow
SDIR=ABC_Shadow_cpp
SRC= $(wildcard $(SDIR)/*.cpp)
OBJ= $(SRC:.cpp=.o)

all: $(EXEC)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

shadow: $(OBJ)
	$(CC) -o $@ $^


clean:
	rm -rf $(SDIR)/*.o
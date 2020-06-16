CC=g++ -std=c++17
CFLAGS=-Wall -Wno-deprecated -O2
EXEC=shadow
SDIR=ABC_Shadow_cpp

SRC= $(wildcard $(SDIR)/*.cpp)
OBJ= $(SRC:.cpp=.o)
INCLUDE=-I$(shell brew --prefix libomp)/include
LIB=-L$(shell brew --prefix libomp)/lib
all: $(EXEC)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS) $(INCLUDE)

shadow: $(OBJ)
	$(CC) -o $@ $^ -I"$(brew --prefix libomp)/include" 


clean:
	rm -rf $(SDIR)/*.o	
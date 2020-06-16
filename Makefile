CC=g++ -std=c++17 -fopenmp
CFLAGS=-Wall -Wno-deprecated -O2
EXEC=shadow
SDIR=ABC_Shadow_cpp
LIB=
INCLUDE=

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CC += -Xpreprocessor
	LIB += -L$(shell brew --prefix libomp)/lib/  -lomp
	INCLUDE += -I$(shell brew --prefix libomp)/include
endif

SRC= $(wildcard $(SDIR)/*.cpp)
OBJ= $(SRC:.cpp=.o)

all: $(EXEC)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS) $(INCLUDE)

shadow: $(OBJ)
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)


clean:
	rm -rf $(SDIR)/*.o	
CXX = g++

CXXFLAGS = -O3 -DNDEBUG -Wno-deprecated -I./ -L./
LINKFLAGS = -lm

SRCS = \
	suc_array.cpp \
	bit_array.cpp \
	var_byte.cpp \

OBJS = $(SRCS:%.cpp=%.o)

all: $(OBJS)

clean:
	rm -f *.o *~

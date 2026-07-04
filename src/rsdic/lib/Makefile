CXX = g++

CXXFLAGS = -O3 -DNDEBUG -Wno-deprecated -I./ -L./
LINKFLAGS = -lm

SRCS = \
	EnumCoder.cpp \
	RSDic.cpp \
	RSDicBuilder.cpp

OBJS = $(SRCS:%.cpp=%.o)

all: $(OBJS)

clean:
	rm -f *.o *~

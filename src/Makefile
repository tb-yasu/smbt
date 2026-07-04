CXX = g++

CXXFLAGS = -O3 -g -DNDEBUG -W -Wall -Wno-deprecated -I./ 

LINKFLAGS = -lm -L./ 

SRCS1 = \
	build.cpp

SRCS2 = \
	search.cpp

OBJS  = ./rsdic/lib/EnumCoder.o \
	./rsdic/lib/RSDic.o \
	./rsdic/lib/RSDicBuilder.o \
	./lib/suc_array.o \
	./lib/bit_array.o \
	./lib/var_byte.o \
	./mbt/multibit_tree.o \
	./smbt_vla/succinct_multibit_tree_vla.o \
	./smbt_trie/succinct_multibit_tree_trie.o

OBJS1 = $(SRCS1:%.cpp=%.o) 
OBJS2 = $(SRCS2:%.cpp=%.o)

all: smbt-build smbt-search

smbt-build: $(OBJS1)
	$(CXX) $(CXXFLAGS) $(OTHERFLAGS) $(OBJS) $(OBJS1) $(LINKFLAGS) -o smbt-build
smbt-search: $(OBJS2)
	$(CXX) $(CXXFLAGS) $(OTHERFLAGS) $(OBJS) $(OBJS2) $(LINKFLAGS) -o smbt-search
debug:
	make all CXXFLAGS="-ggdb -W -Wall -pedantic"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OTHERFLAGS) -c $<

clean:
	rm -f smbt-build smbt-search *.o *~

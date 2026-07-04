CXX = g++

#CXXFLAGS = -g -DNDEBUG -Wno-deprecated -pedantic -ansi -finline-functions -foptimize-sibling-calls -Wcast-qual -Wwrite-strings -Wsign-promo -Wcast-align -Wno-long-long -fexpensive-optimizations -funroll-all-loops -ffast-math -fomit-frame-pointer -pipe -I./ -L./
CXXFLAGS = -O3 -g -DNDEBUG -W -Wall -Wno-deprecated

LINKFLAGS = -lm

default:
	$(CXX) $(CXXFLAGS) -c succinct_multibit_tree_trie.cpp

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OTHERFLAGS) -c $<

clean:
	rm -f *.o *~

CXX = g++

CXXFLAGS = -O3 -DNDEBUG -Wno-deprecated -I./ -L./

LINKFLAGS = -lm

default:
	$(CXX) $(CXXFLAGS) -c multibit_tree.cpp

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OTHERFLAGS) -c $<

clean:
	rm -f *.o *~

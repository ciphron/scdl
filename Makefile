CXX		= 	g++
CXXFLAGS 	= 	-O3 -fopenmp -Wall -pedantic
LDFLAGS 	= 	-ljson
SOURCES 	= 	$(wildcard *.cpp)
HEADERS 	= 	$(wildcard *.h)
OBJECTS 	= 	$(SOURCES:.cpp=.o)
EXEC		= 	eval

all: $(SOURCES) $(EXEC) 

$(EXEC): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(OBJECTS) $(LDFLAGS)

$(OBJECTS): Makefile $(HEADERS) 

clean:
	rm -f $(OBJECTS) $(EXEC)

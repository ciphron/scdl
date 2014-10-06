CPP=g++
CCFLAGS=-O3
LDFLAGS= 
SOURCES=Circuit.cpp SCDLProgram.cpp eval.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=eval
LIBS=

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CPP) $(LDFLAGS) $(LIBS) $(OBJECTS) -o $@

.cpp.o:
	$(CPP) $(CCFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXECUTABLE)

CXX		= 	g++
CXXFLAGS 	= 	-O3 -fopenmp -Wall -pedantic
LDFLAGS 	= 	-ljson
SOURCES 	= 	SCDLProgram.cpp Circuit.cpp SCDLEvaluator.cpp
EVAL_SOURCE	= 	eval.cpp
HEADERS 	= 	$(wildcard *.h)
LIB_OBJECTS 	= 	SCDLProgram.o Circuit.o SCDLEvaluator.o
EVAL_OBJECT	= 	eval.o
LIB		=	libscdl.a
EXEC		= 	eval

all: $(SOURCES) $(EVAL_SOURCE) $(EXEC) $(LIB)

$(EXEC): $(EVAL_OBJECT) $(LIB)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(EVAL_SOURCE) $(LDFLAGS) $(LIB)

$(LIB):	$(SOURCES)
	$(CXX) $(CXXFLAGS) -c $(SOURCES) $(LDFLAGS)
	ar rcs $(LIB) $(LIB_OBJECTS)

$(OBJECTS): Makefile $(HEADERS) 

clean:
	rm -f $(LIB_OBJECTS) $(EVAL_OBJECT) $(EXEC) $(LIB)

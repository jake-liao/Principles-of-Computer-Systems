# Makefile
# hsliao Jake Hsueh-Yu Liao 1551558
ID 	= hsliao
ASNGMNT = asgn0
ADT     = 
EXEBIN  = dog#$(ADT)$(MODE)
CXXFLAGS=-std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -Og
_submit_CXXFLAGS = -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -O2
LDFLAGS =
CXX 	= clang++
SOURCES = $(EXEBIN).c
OBJECTS = $(EXEBIN).o
HEADERS = $(ADT).h
SUBMIT = submit cmps101-pt.u19 $(ASNGMNT)

all: $(EXEBIN)

$(EXEBIN) : $(OBJECTS) #$(HEADERS)
	#gcc -o $(EXEBIN) $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)

$(OBJECTS) : $(SOURCES) #$(HEADERS)
	#gcc -c -g $(FLAGS) $(SOURCES)
	$(CXX) $(CXXFLAGS) -c -MD -o $@ $<

clean :
	rm -f $(EXEBIN) $(OBJECTS) *.exe *.stackdump

check:
	valgrind --leak-check=full $(EXEBIN) 

format:
	clang-format -i $(SOURCES) $(INCLUDES)

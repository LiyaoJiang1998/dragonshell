# /**
#  * Name: Liyao Jiang
#  * ID:
#  */
CC = g++
CFLAGS = -Wall -O2
SOURCES = $(wildcard *.cc)
OBJECTS = $(SOURCES:%.cc=%.o)
SUBMISSION_FILES = dragonshell.cc Makefile readme.md
.PHONY: all clean

all: dragonshell

clean:
	rm *.o dragonshell
	
compile: $(OBJECTS)

%.o: %.cc
	$(CC) $(CFLAGS) -c $^ -o $@

compress: $(SUBMISSION_FILES)
	zip -q -r dragonshell.zip $(SUBMISSION_FILES)

dragonshell: $(OBJECTS)
	$(CC) -o dragonshell $(OBJECTS)
# Makefile for easy compilation

# Compiler plus flags we're using. Note -Wall for all warnings.
CC=gcc -Wall -g -I.

# Calls tets_buddy option
all: test_buddy 

# Links .a file to test_buddy executable
test_buddy: test_buddy.o libbuddy.a
	$(CC) -o test_buddy test_buddy.o -L. -lbuddy

# Uses buddy.o to create archive file library
libbuddy.a: buddy.o
	ar ruv libbuddy.a  buddy.o
	ranlib libbuddy.a

# Creates the buddy object file from .c and .h files
buddy.o: buddy.c buddy.h 
	$(CC) -c buddy.c

# Removes object files
clean:
	rm *.o

# Removes object files + executables
wipe:
	rm *.o *.a test_buddy 

all: seqasm seqdis seqvm

CXXFLAGS=-I ../src
CFLAGS=-I ../src

seqasm:  lex.yy.o seq.tab.o seqasm.o
	$(CXX) $(CXXFLAGS) -o $@ seqasm.o seq.tab.o lex.yy.o -lfl

seqdis: seqdis.o
	gcc -o $@ seqdis.o

seqvm: seqvm.o
	$(CXX) $(CXXFLAGS) -o $@ seqvm.o

seqvm.o: ../src/seqvm.cpp
	$(CXX) $(CXXFLAGS) -c ../src/seqvm.cpp

seqasm.o: seqasm.c seqasm.h seq.tab.h
	$(CC) $(CFLAGS) -c seqasm.c

seqdis.o: seqdis.c
	$(CC) $(CFLAGS) -c seqdis.c

lex.yy.o: lex.yy.c seq.tab.o
	$(CC) $(CFLAGS) -c lex.yy.c

seq.tab.o: seq.tab.c seq.tab.h seqasm.h
	$(CC) $(CFLAGS) -c seq.tab.c

seq.tab.c: seq.y
	bison -d -v -Wconflicts-sr seq.y

lex.yy.c: seq.l
	flex seq.l

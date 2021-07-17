all: seqasm seqdis seqvm
clean:
	rm -f *.o lex.yy.c seq.tab.c *.bin

CXXFLAGS=-I ../src
CFLAGS=-I ../src
LANGHEADER=../src/seqlang.h

seqasm:  lex.yy.o seq.tab.o seqasm.o
	$(CXX) $(CXXFLAGS) -o $@ seqasm.o seq.tab.o lex.yy.o -lfl

seqdis: seqdis.o
	gcc -o $@ seqdis.o

seqvm: seqvm.o hardware.o
	$(CXX) $(CXXFLAGS) -o $@ seqvm.o hardware.o

seqvm.o: ../src/seqvm.cpp $(LANGHEADER)
	$(CXX) $(CXXFLAGS) -c ../src/seqvm.cpp

hardware.o: ../src/hardware.cpp $(LANGHEADER)
	$(CXX) $(CXXFLAGS) -c ../src/hardware.cpp

seqasm.o: seqasm.c seqasm.h seq.tab.h $(LANGHEADER)
	$(CC) $(CFLAGS) -c seqasm.c

seqdis.o: seqdis.c $(LANGHEADER)
	$(CC) $(CFLAGS) -c seqdis.c

lex.yy.o: lex.yy.c seq.tab.o
	$(CC) $(CFLAGS) -c lex.yy.c

seq.tab.o: seq.tab.c seq.tab.h seqasm.h
	$(CC) $(CFLAGS) -c seq.tab.c

seq.tab.c: seq.y
	bison -d -Wconflicts-sr seq.y

lex.yy.c: seq.l
	flex seq.l

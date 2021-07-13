CC = gcc
LNK = gcc
LNKFLAGS = -lOpenCL
CCFLAGS = -g
OBJ = main.o mult.o

clean:   
	rm *.o

cleanall: clean
	rm prog.out

%.o: %.c
	$(CC) -c $(CCFLAGS) -o $@ $<

prog.out: $(OBJ) 
	$(LNK) $(LNKFLAGS) -o $@ $?

all: prog.out

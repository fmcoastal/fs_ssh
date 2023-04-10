
all:    fs_ssh


# sudo apt install libssh-dev




CCFLAGS = -c -g -O0 -D_GNU_SOURCE -Wall -pthread 
LDFLAGS = -Wl,-v -Wl,-Map=a.map -Wl,--cref -Wl,-t -lpthread -pthread
#LDFLAGS = -lpthread -pthread
ARFLAGS = -rcs

# buildroot passes these in
CC = gcc
LD = gcc
AR = ar

fs_ssh: fs_ssh.o frbuff.o ftty.o 
	$(LD) $(LDFLAGS1)  -o fs_ssh fs_ssh.o  frbuff.o ftty.o  -lssh -lpthread


ftimeout: ftimeout.o
	$(LD) $(LDFLAGS) -o ftimeout ftimeout.o 

fs_expect: fln_serial.o Makefile ftty.o main.o ffile.o frbuff.o fprintbuff.o
	$(LD) $(LDFLAGS) -o fs_expect ftty.o fln_serial.o main.o ffile.o fprintbuff.o frbuff.o 

main.o: main.c
	$(CC)  $(CCFLAGS) -o main.o  main.c
fln_serial.o: fln_serial.c fln_serial.h
	$(CC)  $(CCFLAGS) -o fln_serial.o  fln_serial.c
ffile.o: ffile.c  ffile.h
	$(CC)  $(CCFLAGS) -o ffile.o  ffile.c
frbuff.o: frbuff.c frbuff.h
	$(CC)  $(CCFLAGS) -o frbuff.o  frbuff.c
ftty.o: ftty.c ftty.h
	$(CC)  $(CCFLAGS) -o ftty.o  ftty.c
ftimeout.o: ftimeout.c 
	$(CC)  $(CCFLAGS) -o ftimeout.o  ftimeout.c



fprintbuff.o: fprintbuff.c fprintbuff.h
	$(CC)  $(CCFLAGS) -o fprintbuff.o  fprintbuff.c

fs_ssh.o: fs_ssh.c 
	$(CC)  $(CCFLAGS) -o fs_ssh.o  fs_ssh.c





clean:
	rm -rf *.o
	rm -rf *.map
	rm -rf fs_expect
	rm -rf fs_ssh
	rm -rf ftimeout

.phony x:
x:
	./fs_expect



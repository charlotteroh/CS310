CS 310 P2: Black Hat Lab
Author: Charlotte Roh (hr67), John Kang (jk307)
Date: November 5, 2019
Time spent: 10 hours

(Serverport: http://310test.cs.duke.edu:9180 Killport: http://310test.cs.duke.edu:9181)

1. Stack-smash vulnerability in the server code.

The stack-smash vulnerability in the server code is the fact that the
"int check_filename_length(byte len)" method takes bytes as parameters, whereas
an int was passed in the main function. The data type has been defined as
equivalent to unsigned character data type, which only allows number representation
from 0 to 255. Thus, in C, only the last 8 bits would be passed, while the rest
are gotten rid of. Therefore, the server only would  check if the file length is
less than 100 for the last 8 bytes, not the whole file length. This means that
we can use a longer string of filename as a parameter and still pass the filename
length checker method.

2. Method of attack

The point of this whole lab was to compromise a server using shellcode through
the stack overflow attack. Particularly, we used the reverse shellcode to
attack the loophole in the server so that we can remotely send commands to the
server after taking it over.

Our shellcode.c allows us to create an attack string composed of three
components: the return address, no operation instruction (NOP's), and shellcode
(which contains string representation of assembly instructions that opens up
a shell that is bound to a particular port on a remote server). We used port
19180 (0xec4a) in our shellcode.

Our final payload looks like the following:

|---------|-------------------|---------|----------------------|-----------------|
  NOP pad   address (140-170)   NOP pad   shellcode (92 bytes)   NOP pad (0-864)

We used 210 as the offset of our return address, which was intentionally chosen
so that it is slightly smaller than the length of our NOP block. Through
iteration of logical guesses, we were able to make the remote server to execute
the shellcode that allowed us to excess its shell via netcat.

All the linux commands we've used are compiled in instructions.txt. The final
hacked server screen is shown in hack.jpg.

Overall, it was a very interesting and intriguing assignment, as we were able to
modify a server without having access to it. We also learned that hacking is a
very difficult and laborsome task, so we decided to stick with being good people.

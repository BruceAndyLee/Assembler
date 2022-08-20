# Assembler

This program here is supposed to be able to read assembler-like code (with some vast simplifications) and compile it into a series of commands for a compatible processor program to execute the resulting "machine"-code

Asm commands supported
Stack:
- jump
- push
- pop
- add
- mul
- sub

Function stack:
- call
- ret

Registers:
- outr
- inr
- in
- out

Flow control:
- HLT
- JE
- JG
- JL
- CMP

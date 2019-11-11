Make Baby
=========

The shittiest make of all time.

Supports `Makebabyfile`s in this format:

    lib.o: lib.c
        gcc -c -o lib.o lib.c

    program: program.o lib.o
        gcc -o program program.o lib.o

    program.o: program.c
        gcc -c -o program.o program.c

Run `../makebaby` inside `testbed/` (😉) to test it

Does not check mtimes... yet(???)

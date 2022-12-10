all:
	gcc -o 6502emu main.c 6502emu.c
save:
	gcc -o 6502emu main.c 6502emu.c -save-temps

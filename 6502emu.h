//
//  6502emu.h
//  6502emu
//
//  Created by Mattia Furlan on 04/09/15.
//  Copyright © 2015 Mattia Furlan. All rights reserved.
//

#ifndef _502emu_h
#define _502emu_h
#include <stdint.h>
#define RAM_SIZE 0x10000 /* 64k RAM */
#define STACK_DEFAULT_VALUE 0xff
#define MAKE_STACK(__sp) (0x0100 | __sp)
#define DEFAULT_LOADING_POINT 0x5000
#define DEFAULT_CLOCK_FREQUENCY 1000000
#define SCREEN_BASE 0xF000 /* 2000 bytes for screen (0xf000 ~ 0xf7d0) */
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define GET_BIT(__byte, __n) (((__byte) >> (__n)) & 1) /* Get the '_n'-th bit in a byte */
#define SET_BIT(__byte, __n, __val) ((__byte) ^= (-(__val) ^ (__byte)) & (1 << (__n))) /* Set the '_n'th bit in a byte */
typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
#ifndef __cplusplus
typedef enum bit {false = 0, true = 1} bit, bool;
#else
typedef bool bit;
#endif

typedef struct cpu
{
    /* Registers */
    byte A;
    byte X;
    byte Y;
    byte SP; /* Stack pointer */
    word PC; /* Program counter */
    /* Processor flags */
    bit C;          //0) C: carry flag
    bit Z;          //1) Z: zero flag (1 = last result is 0)
    bit Int;        //2) I: interrupts flag (1 = disabled)
    bit D;          //3) D: Decimal mode
    bit B;          //4) B: (1 = interrupt executed)
    bit V;          //6) V: Overflow flag
    bit S;          //7) S: Sign flag (1 = negative)
    bit useValue;
    bit useAcc;
    byte operand;
    word address;
    bit halt;
    dword size;
    char screen [80*25];
    char cursor;

} cpu, *cpu_ptr;

typedef void (func)(cpu_ptr cpu);
typedef uint16_t (address)(cpu_ptr cpu);

void cpu_reset (cpu_ptr cpu);
int  cpu_execute (cpu_ptr cpu);
void set_carry_flag (cpu_ptr cpu, bit value);
void set_zero_flag (cpu_ptr cpu, byte value);
void set_int_flag (cpu_ptr cpu, bit value);
void set_dec_flag (cpu_ptr cpu, bit value);
void set_int_used_flag (cpu_ptr cpu, bit value);
void set_overflow_flag (cpu_ptr cpu, bit value);
void set_sign_flag (cpu_ptr cpu, bit value);

void print_registers (cpu_ptr cpu);
void print_screen ();

#endif /* _502emu_h */

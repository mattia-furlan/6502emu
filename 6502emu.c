//
//  6502emu.c
//  6502emu
//
//  Created by Mattia Furlan on 04/09/15.
//  Copyright © 2015 Mattia Furlan. All rights reserved.
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "6502emu.h"

extern byte RAM[0x10000];
extern word LOADING_POINT;
extern dword CLOCK_F;
//extern char screen[SCREEN_HEIGHT][SCREEN_WIDTH];
extern char screen[2000];
extern bool enable_exts;
int cursor;

FILE *out;

void print_bits (word w)
{
    for (int i=0;i<16;i++)
    {
        printf ("%d", GET_BIT (w, (15-i)));
        if (((i+1) % 4) == 0)
            printf (" ");
    }
    printf ("\n");
}

void cpu_reset (cpu_ptr cpu)
{
    /* Registers */
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = STACK_DEFAULT_VALUE;
    cpu->PC = LOADING_POINT;
    /* Flags */
    cpu->D = 0;
    cpu->Z = 0;
    cpu->V = 0;
    cpu->B = 0;
    cpu->C = 0;
    cpu->Int = 0;
    cpu->S = 0;
    cpu->halt = 0;
    cpu->useValue = false;
    cpu->useAcc = false;
}

void print_registers (cpu_ptr cpu)
{
    /*printf("A:  0x%02x | X:  0x%02x | Y: 0x%02x | ", cpu->A, cpu->X, cpu->Y);
    printf("PC: 0x%04x | SP: 0x01%02x\n", cpu->PC, cpu->SP);
    printf ("NV-BDIZC\n");
    printf ("%d%d-%d%d%d%d%d\n", cpu->S, cpu->V, cpu->B, cpu->D, cpu->Int, cpu->Z, cpu->C);*/
    byte sr;
    SET_BIT (sr, 0, cpu->C); SET_BIT (sr, 1, cpu->Z); SET_BIT (sr, 2, cpu->Int); SET_BIT (sr, 3, cpu->D);
    SET_BIT (sr, 4, cpu->B); SET_BIT (sr, 5,    1  ); SET_BIT (sr, 6,  cpu->V ); SET_BIT (sr, 7, cpu->S);
    printf("==============================\n");
    printf (" PC  AC XR YR SR SP   NV-BDIZC\n\n");
    printf ("%04X %02X %02X %02X %02X %02X", cpu->PC, cpu->A, cpu->X, cpu->Y, sr, cpu->SP);
    printf ("   %d%d1%d%d%d%d%d\n", cpu->S, cpu->V, cpu->B, cpu->D, cpu->Int, cpu->Z, cpu->C);
    printf("==============================\n\n");
}


/********************************************************/
/**              MEMORY ADDRESSING MODES               **/
/********************************************************/

/* Accumulator address mode */
word acc(cpu_ptr cpu)
{
    printf("addr_mode: ACC\n");
    cpu->useAcc = true;
    return cpu->A;
}

/* Implicit address mode */
word imp(cpu_ptr cpu)
{
    printf("addr_mode: IMP\n");
    return 0;
}

/* Immediate */
word imm(cpu_ptr cpu)
{
    printf("addr_mode: IMM\n");
    cpu->PC++;
    byte addr = RAM[cpu->PC];
    cpu->useValue = true;
    return addr;
}

/* Zero page address mode */
word zp(cpu_ptr cpu)
{
    printf("addr_mode: ZP\n");
    cpu->PC++;
    byte address = RAM[cpu->PC];
    return (word)address;
}

/* Zero page, X address mode */
word zpx(cpu_ptr cpu)
{
    printf("addr_mode: ZPX\n");
    cpu->PC++;
    byte address = cpu->X + RAM[cpu->PC];
    return (word)address;
}

/* Zero page, Y address mode */
word zpy(cpu_ptr cpu)
{
    printf("addr_mode: ZPY\n");
    cpu->PC++;
    byte address = cpu->Y + RAM[cpu->PC];
    return (word)address;
}

/* Absolute address mode */
word abso(cpu_ptr cpu)
{
    printf("addr_mode: ABSO\n");
    cpu->PC++;
    word high_address = RAM[cpu->PC+1];
    high_address <<= 8;
    word low_address = RAM[cpu->PC];
    cpu->PC++;
    printf("%02x\n", low_address);
    high_address |= low_address;
    return high_address;
}

/* Absolute, X address mode */
word absx(cpu_ptr cpu)
{
    printf("addr_mode: ABSX\n");
    cpu->PC++;
    word high_address = RAM[cpu->PC+1];
    high_address <<= 8;
    word low_address = RAM[cpu->PC];
    cpu->PC++;
    high_address |= low_address;
    high_address += cpu->X;
    return high_address;
}

/* Absolute, Y address mode */
word absy(cpu_ptr cpu)
{
    printf("addr_mode: ABSY\n");
    cpu->PC++;
    word high_address = RAM[cpu->PC+1];
    high_address <<= 8;
    byte low_address = RAM[cpu->PC];
    cpu->PC++;
    high_address |= low_address;
    high_address += cpu->Y;
    return high_address;
}

/* Indirect address mode */
word ind(cpu_ptr cpu)
{
    printf("addr_mode: IND\n");
    cpu->PC++;
    word high_address = RAM[cpu->PC+1];
    high_address <<= 8;
    byte low_address = RAM[cpu->PC];
    cpu->PC++;
    high_address |= low_address;
    printf ("%02x\n", high_address);
    word final_high_address = RAM[high_address+1];
    final_high_address <<= 8;
    byte final_low_address = RAM[high_address];
    return (final_high_address |= final_low_address);
    
}

/* Indirect, X address mode */
word indx(cpu_ptr cpu)
{
    printf ("addr_mode: INDX\n");
    cpu->PC++;
    byte address = RAM[cpu->PC];
    printf("address = %02x\n", address);
    address += cpu->X;
    printf("address + X = %02x\n", address);
    word high_address = RAM[address+1];
    //printf("high_address = %02x\n", high_address);
    high_address <<= 8;
    //printf("high_address << 8 = %02x\n", high_address);
    word low_address = RAM[address];
    high_address |= low_address;
    return high_address;
}

/* Indirect, Y address mode */
word indy(cpu_ptr cpu)
{
    printf ("addr_mode: INDY\n");
    cpu->PC++;
    byte address = RAM[cpu->PC];
    printf ("address = %02x\n", address);
    word high_address = RAM[address + 1];
    //printf ("high_address = %02x\n", high_address);
    high_address <<= 8;
    //printf ("high_address << 8 = %02x\n", high_address);
    word low_address = RAM[address];
    //printf ("low_address = %02x\n", low_address);
    high_address |= low_address;
    //printf ("address = %02x\n", high_address);
    high_address += cpu->Y;
    return high_address;
}

/* Relative address mode */
word rel(cpu_ptr cpu)
{
    printf("addr_mode: REL\n");
    cpu->PC++;
    word address;
    byte offset = RAM[cpu->PC];
    printf("offset = %d\n", (offset<128?offset:((256-offset)*(-1))));
    if (offset < 128)
        address = cpu->PC + offset;
    else
        address = cpu->PC + ((256 - offset)*(-1));
    return address;
}

/********************************************************/
/**                   INSTRUCTIONS                     **/
/********************************************************/

/*void adc (cpu_ptr cpu)
{
    printf("adc\n");
    word sum = cpu->A + cpu->operand + cpu->C;
    byte buffer = (byte)sum;
    //printf("A = %d, op = %d, C = %d, sum = %d\n", cpu->A, cpu->operand, cpu->C, sum);
    bit sign_a = GET_BIT(cpu->A, 7);
    bit sign_op = GET_BIT(cpu->operand, 7);
    bit sign_sum = GET_BIT(buffer, 7);
    
    if((sign_a == sign_op) && (sign_a != sign_sum))
    {
        set_overflow_flag(cpu, 1);
    }
    else
        set_overflow_flag(cpu, 0);
    
    cpu->A = (byte)sum;
    //printf("(after)A = %d, op = %d, C = %d, sum = %d\n", cpu->A, cpu->operand, cpu->C, sum);
    
    set_zero_flag(cpu, cpu->A);
    set_sign_flag(cpu, GET_BIT (cpu->A, 7));
    set_carry_flag (cpu, sum > 255); //overflow
}*/

void adc (cpu_ptr cpu)
{
    printf ("adc\n");
    word result = (word)cpu->A + cpu->operand + (word)(cpu->C);
    
    //carrycalc(result);
    //zerocalc(result);
    //overflowcalc(result, a, value);
    //signcalc(result);
    set_carry_flag (cpu, result > 255);
    set_zero_flag (cpu, result);
    //set_overflow_flag (cpu, (((result) ^ (word)(cpu->A)) & ((result) ^ (cpu->operand)) & 0x0080));
    set_overflow_flag (cpu, GET_BIT(cpu->A, 7) != GET_BIT (result, 7));
    set_sign_flag (cpu, GET_BIT (result, 7));
    
    cpu->A = (byte) result;
}

void And (cpu_ptr cpu)
{
    printf("and\n");
    cpu->A &= cpu->operand;
    set_zero_flag (cpu, cpu->A);
    set_sign_flag (cpu, GET_BIT (cpu->A, 7)); /* Set negative(sign) flag if the 7th bit = 1 */
}

void asl (cpu_ptr cpu)
{
    printf("asl\n");
    set_carry_flag (cpu, GET_BIT (cpu->operand, 7)); /* Carry if the number is too big */
    cpu->operand <<= 1;
    set_zero_flag(cpu, cpu->operand);
    set_sign_flag (cpu, GET_BIT (cpu->operand, 7));
}

void bcc (cpu_ptr cpu)
{
    printf("bcc\n");
    if(cpu->C == 0)
        cpu->PC = cpu->address;
}

void bcs (cpu_ptr cpu)
{
    printf("bcs\n");
    if(cpu->C == 1)
        cpu->PC = cpu->address;
}

void beq (cpu_ptr cpu)
{
    printf("beq\n");
    if(cpu->Z == 1)
        cpu->PC = cpu->address;
}

void Bit (cpu_ptr cpu)
{
    printf("bit\n");
    byte buffer = cpu->A & cpu->operand;
    set_zero_flag(cpu, buffer);
    set_overflow_flag(cpu, GET_BIT(cpu->operand, 6));
    set_sign_flag(cpu, GET_BIT(cpu->operand, 7));
}

void bmi (cpu_ptr cpu)
{
    printf("bmi\n");
    if(cpu->S == 1)
        cpu->PC = cpu->address;
}

void bne (cpu_ptr cpu)
{
    printf("bne\n");
    if(cpu->Z == 0)
        cpu->PC = cpu->address;
}

void bpl (cpu_ptr cpu)
{
    printf("bpl\n");
    if(cpu->S == 0)
        cpu->PC = cpu->address;
}

void php (cpu_ptr);

void Brk (cpu_ptr cpu)
{
    printf("brk\n");
    byte low_pc = cpu->PC & 0xff;
    byte high_pc = (cpu->PC & 0xff00) >> 8;
    RAM[MAKE_STACK (cpu->SP)] = low_pc;
    cpu->SP--;
    RAM[MAKE_STACK (cpu->SP)] = high_pc;
    cpu->SP--;
    
    /* Push flags register */
    php(cpu);
    set_int_used_flag(cpu, 1);
}

void bvc (cpu_ptr cpu)
{
    printf("bvc\n");
    if(cpu->V == 0)
        cpu->PC = cpu->address;
}

void bvs (cpu_ptr cpu)
{
    printf("bvs\n");
    if(cpu->V == 1)
        cpu->PC = cpu->address;
}

void clc (cpu_ptr cpu)
{
    printf("clc\n");
    set_carry_flag (cpu, 0);
}

void cld (cpu_ptr cpu)
{
    printf("cld\n");
    set_dec_flag (cpu, 0);
}

void cli (cpu_ptr cpu)
{
    printf("cli\n");
    set_int_flag (cpu, 0);
}

void clv (cpu_ptr cpu)
{
    printf("clv\n");
    set_overflow_flag (cpu, 0);
}

void cmp (cpu_ptr cpu)
{
    printf("cmp\n");
    byte result = cpu->A - cpu->operand;
    printf("A = %02x, op=%02x\n", cpu->A, cpu->operand);
    set_carry_flag(cpu, (cpu->A >= cpu->operand?1:0));
    set_zero_flag(cpu, result);
    set_sign_flag(cpu, GET_BIT(result, 7));
}

void cpx (cpu_ptr cpu)
{
    printf("cpx\n");
    byte result = cpu->X - cpu->operand;
    set_carry_flag(cpu, (cpu->X >= cpu->operand?1:0));
    set_zero_flag(cpu, result);
    set_sign_flag(cpu, GET_BIT(result, 7));
}

void cpy (cpu_ptr cpu)
{
    printf("cpy\n");
    byte result = cpu->Y - cpu->operand;
    set_carry_flag(cpu, (cpu->Y >= cpu->operand?1:0));
    set_zero_flag(cpu, result);
    set_sign_flag(cpu, GET_BIT(result, 7));
}

void dec (cpu_ptr cpu)
{
    printf("dec\n");
    cpu->operand--;
    set_zero_flag(cpu, cpu->operand);
    set_sign_flag(cpu, GET_BIT(cpu->operand, 7));
}

void dex (cpu_ptr cpu)
{
    printf("dex\n");
    cpu->X--;
    set_zero_flag (cpu, cpu->X);
    set_sign_flag (cpu, GET_BIT(cpu->X, 7));
}

void dey (cpu_ptr cpu)
{
    printf("dey\n");
    cpu->Y--;
    set_zero_flag (cpu, cpu->Y);
    set_sign_flag (cpu, GET_BIT(cpu->Y, 7));
}

void eor (cpu_ptr cpu)
{
    printf("eor\n");
    cpu->A ^= cpu->operand;
    set_zero_flag(cpu, cpu->A);
    set_sign_flag(cpu, GET_BIT(cpu->A, 7));
}

void inc (cpu_ptr cpu)
{
    printf("inc\n");
    cpu->operand++;
    set_zero_flag(cpu, cpu->operand);
    set_sign_flag(cpu, GET_BIT(cpu->operand, 7));
}

void inx (cpu_ptr cpu)
{
    printf("inx\n");
    cpu->X++;
    set_zero_flag(cpu, cpu->X);
    set_sign_flag(cpu, GET_BIT(cpu->X, 7));
}

void iny (cpu_ptr cpu)
{
    printf("iny\n");
    cpu->Y++;
    set_zero_flag(cpu, cpu->Y);
    set_sign_flag(cpu, GET_BIT(cpu->Y, 7));
}

void jmp (cpu_ptr cpu)
{
    printf("jmp\n");
    cpu->PC = cpu->address-1;
}

void jsr (cpu_ptr cpu)
{
    printf("jsr\n");
    printf("PC = %02x\n", cpu->PC);
    byte low_pc = cpu->PC & 0xff;
    byte high_pc = (cpu->PC & 0xff00) >> 8;
    RAM[MAKE_STACK(cpu->SP)] = low_pc;
    cpu->SP--;
    RAM[MAKE_STACK(cpu->SP)] = high_pc;
    cpu->SP--;
    printf("%02x\n", cpu->address);
    cpu->PC = cpu->address-1;
}

void lda (cpu_ptr cpu)
{
    printf("lda\n");
    cpu->A = cpu->operand;
    set_zero_flag(cpu, cpu->A);
    set_sign_flag(cpu, GET_BIT(cpu->A, 7));
}

void ldx (cpu_ptr cpu)
{
    printf("ldx\n");
    cpu->X = cpu->operand;
    set_zero_flag(cpu, cpu->X);
    set_sign_flag(cpu, GET_BIT(cpu->X, 7));
}

void ldy (cpu_ptr cpu)
{
    printf("ldy\n");
    cpu->Y = cpu->operand;
    set_zero_flag(cpu, cpu->Y);
    set_sign_flag(cpu, GET_BIT(cpu->Y, 7));
}

void lsr (cpu_ptr cpu)
{
    printf("lsr\n");
    set_carry_flag (cpu, GET_BIT (cpu->operand, 0));
    cpu->operand >>= 1;
    cpu->operand &= 0x80; //safer?
    set_zero_flag(cpu, cpu->operand);
    set_sign_flag(cpu, GET_BIT(cpu->operand, 7));
}

void nop (cpu_ptr cpu)
{
    printf("nop\n");
}

void ora (cpu_ptr cpu)
{
    printf("ora\n");
    cpu->A |= cpu->operand;
    set_zero_flag(cpu, cpu->A);
    set_sign_flag(cpu, GET_BIT(cpu->A, 7));
}

void pha (cpu_ptr cpu)
{
    printf("pha\n");
    RAM[MAKE_STACK(cpu->SP)] = cpu->A;
    cpu->SP--;
}

#define NOT_USED 1

void php (cpu_ptr cpu)
{
    printf("php\n");
    byte buffer = cpu->C;
    buffer |= (cpu->Z << 1);
    buffer |= (cpu->Int << 2);
    buffer |= (cpu->D << 3);
    buffer |= (cpu->B << 4);
    buffer |= (NOT_USED << 5);
    buffer |= (cpu->V << 6);
    buffer |= (cpu->S << 7);
    print_bits (buffer);
    RAM[MAKE_STACK(cpu->SP)] = buffer;
    cpu->SP--;
}

void pla (cpu_ptr cpu)
{
    printf("pla\n");
    cpu->SP++;
    cpu->A = RAM[MAKE_STACK(cpu->SP)];
    set_zero_flag(cpu, cpu->A);
    set_sign_flag(cpu, GET_BIT(cpu->A, 7));
}

void plp (cpu_ptr cpu)
{
    printf("plp\n");
    cpu->SP++;
    byte buffer = RAM[MAKE_STACK(cpu->SP)];
    printf ("buffer = %02x\n", buffer);
    /*
    cpu->C = buffer & 1;
    cpu->Z = (buffer & 2) >> 1;
    cpu->Int = (buffer & 4) >> 2;
    cpu->D = (buffer & 8) >> 3;
    cpu->B = (buffer & 16) >> 4;
    cpu->V = (buffer & 32) >> 5;
     cpu->S = (buffer & 128) >> 7;*/
    print_bits (buffer);
    cpu->C = GET_BIT (buffer, 0);
    cpu->Z = GET_BIT (buffer, 1);
    cpu->Int = GET_BIT (buffer, 2);
    cpu->D = GET_BIT (buffer, 3);
    cpu->B = GET_BIT (buffer, 4);
    cpu->V = GET_BIT (buffer, 6);
    cpu->S = GET_BIT (buffer, 7);
}

/************/
void rol (cpu_ptr cpu)
{
    printf("rol\n");
    byte old_carry_flag = cpu->C;
    byte old_bit_7 = (GET_BIT(cpu->operand, 7));
    cpu->operand <<= 1;
    cpu->operand ^= (-(old_carry_flag==0?0:1) ^ cpu->operand) & 1;
    set_carry_flag(cpu, old_bit_7); /* Carry flag = old bit 7 */
    set_zero_flag(cpu, cpu->A);
}

/************/
void ror (cpu_ptr cpu)
{
    printf("ror\n");
    byte old_carry_flag = cpu->C;
    byte old_bit_0 = (GET_BIT(cpu->operand, 0));
    cpu->operand >>= 1;
    //cpu->operand &= 1 << old_carry_flag; /* Bit 0 = old carry flag */
    cpu->operand ^= (-(old_carry_flag==0?0:1) ^ cpu->operand) & (1 << 7);
    set_carry_flag(cpu, old_bit_0); /* Carry flag = old bit 7 */
    set_zero_flag(cpu, cpu->A);
}

void rti (cpu_ptr cpu)
{
    printf("rti\n");
    cpu->SP++;
    byte buffer = RAM[MAKE_STACK(cpu->SP)];
    cpu->SP++;
    word low_pc = RAM[MAKE_STACK(cpu->SP)];
    cpu->SP++;
    word high_pc = RAM[MAKE_STACK(cpu->SP)];
    word new_pc = (high_pc << 8) | low_pc;
    printf("new_pc = %x\n", new_pc);
    cpu->PC = new_pc - 1;
    cpu->C = buffer & 1;
    cpu->Z = (buffer & 2) >> 1;
    cpu->Int = (buffer & 4) >> 2;
    cpu->D = (buffer & 8) >> 3;
    cpu->B = (buffer & 16) >> 4;
    cpu->V = (buffer & 32) >> 5;
    cpu->S = (buffer & 64) >> 6;
}

void rts (cpu_ptr cpu)
{
    printf("rts\n");
    cpu->SP++;
    word high_pc = RAM[MAKE_STACK(cpu->SP)];
    cpu->SP++;
    word low_pc = RAM[MAKE_STACK(cpu->SP)];
    word pc = (high_pc << 8) | low_pc;
    printf("pc = %02x\n", pc);
    cpu->PC = pc;
}

void sbc (cpu_ptr cpu)
{
    printf("sbc\n");
    cpu->operand = ~cpu->operand;
    adc(cpu);
}

void sec (cpu_ptr cpu)
{
    printf("sec\n");
    set_carry_flag (cpu, 1);
}

void sed (cpu_ptr cpu)
{
    printf ("sed\n");
    set_dec_flag(cpu, 1);
}

void sei (cpu_ptr cpu)
{
    printf("sei\n");
    set_int_flag (cpu, 1);
}

/*
// Undocumented instructions
void slo (cpu_ptr cpu){}
void rla (cpu_ptr cpu){}
void rra (cpu_ptr cpu){}
void sre (cpu_ptr cpu){}
*/

void sta (cpu_ptr cpu)
{
    printf("sta\n");
    cpu->operand = cpu->A;
    fprintf (out, "(%04X): %c\n", cpu->operand, cpu->A);
}

void stx (cpu_ptr cpu)
{
    printf("stx\n");
    cpu->operand = cpu->X;
    fprintf (out, "(%04X): %c\n", cpu->operand, cpu->X);
}

void sty (cpu_ptr cpu)
{
    printf("sty\n");
    cpu->operand = cpu->Y;
    fprintf (out, "(%04X): %c", cpu->operand, cpu->Y);
}

void tax (cpu_ptr cpu)
{
    printf("tax\n");
    cpu->X = cpu->A;
    set_zero_flag(cpu, cpu->X);
    set_sign_flag(cpu, GET_BIT(cpu->X, 7));
}

void tay (cpu_ptr cpu)
{
    printf("tay\n");
    cpu->Y = cpu->A;
    set_zero_flag(cpu, cpu->Y);
    set_sign_flag(cpu, GET_BIT(cpu->Y, 7));
}

void tsx (cpu_ptr cpu)
{
    printf("tsx\n");
    cpu->X = cpu->SP;
    set_zero_flag(cpu, cpu->X);
    set_sign_flag(cpu, GET_BIT(cpu->X, 7));
}

void txa (cpu_ptr cpu)
{
    printf("txa\n");
    cpu->A = cpu->X;
    set_zero_flag(cpu, cpu->A);
    set_sign_flag(cpu, GET_BIT(cpu->A, 7));
}

void txs (cpu_ptr cpu)
{
    printf("txs\n");
    cpu->SP = cpu->X;
}

void tya (cpu_ptr cpu)
{
    printf("tya\n");
    cpu->A = cpu->Y;
    set_zero_flag(cpu, cpu->A);
    set_sign_flag(cpu, GET_BIT(cpu->A, 7));
}


/*
 Undocumented instructions
void sax (cpu_ptr cpu){}
void lax (cpu_ptr cpu){}
void dcp (cpu_ptr cpu){}
void isb (cpu_ptr cpu){}
 */

#include "tables.h"

int cpu_execute (cpu_ptr cpu)
{
    out = fopen ("screen.txt", "w");
    byte opcode;
    word address;
    cursor=0;
    while (1)
    {
        opcode = RAM[cpu->PC];
        printf ("opcode = %02x\n", opcode);
        address = addr_modes[opcode] (cpu);
        printf ("address = %04x\n------------\n", address);
        if (cpu->useValue || cpu->useAcc)
        {
            cpu->operand = address;
        }
        else
            cpu->operand = RAM[address];
        cpu->address = address;
        if (opcode == 0x00)
            break;
        (opcodes[opcode]) (cpu);
        printf ("operand(usev=%d, useA=%d) = %02x\n", cpu->useValue, cpu->useAcc, cpu->operand);
        if (cpu->useValue)
        {
            cpu->useValue = 0;
        }
        else if (cpu->useAcc)
        {
            cpu->A = cpu->operand;
            cpu->useAcc = 0;
        }
        else
            RAM[address] = cpu->operand;
        cpu->PC++;
        print_registers (cpu);
        /*if(address == 0x0205 || address == 0x0207) // Update screen
        {
            fprintf (out, "%c", RAM[0xf001]);
            while (1);
        }*/
        usleep (1000000/CLOCK_F);
        /*int d;
        AGAIN:
        scanf ("%d", &d);
        if (d == 1)
        {
            int addr;
            scanf ("%04x", &addr);
            //printf ("%04X: %02X %02X %02X %02X\n", addr, RAM[addr], RAM[addr + 1], RAM[addr + 2], RAM[addr + 3]);
            //printf ("%04X: %02X %02X %02X %02X\n\n", addr + 4, RAM[addr + 4], RAM[addr + 5], RAM[addr + 6], RAM[addr + 7]);
            for (int i = 0; i < 2; i++, addr += 8)
            {
                printf ("%04X:", addr);
                for (int j = 0; j < 8; j++)
                    printf (" %02X", RAM[addr + j]);
                printf ("\n");
            }
            goto AGAIN;
        }
        printf ("\n");*/
    }
    //print_screen();
    fclose (out);
    return 0;
}

void set_carry_flag (cpu_ptr cpu, bit value)
{
    cpu->C = value;
}

void set_zero_flag(cpu_ptr cpu, byte value)
{
    if(value == 0)
        cpu->Z = 1;
    else
        cpu->Z = 0;
}

void set_int_flag(cpu_ptr cpu, bit value)
{
    cpu->Int = value;
}

void set_dec_flag(cpu_ptr cpu, bit value)
{
    cpu->D = value;
}

void set_int_used_flag(cpu_ptr cpu, bit value)
{
    cpu->B = value;
}

void set_overflow_flag(cpu_ptr cpu, bit value)
{
    cpu->V = value;
}

void set_sign_flag(cpu_ptr cpu, bit value)
{
    cpu->S = value;
}

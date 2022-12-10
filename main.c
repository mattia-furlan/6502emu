//
//  main.c
//  6502emu
//
//  Created by Mattia Furlan on 04/09/15.
//  Copyright © 2015 Mattia Furlan. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include "6502emu.h"

char screen[SCREEN_HEIGHT*SCREEN_WIDTH];

byte RAM[RAM_SIZE]; /* 64KB RAM  */
word LOADING_POINT = DEFAULT_LOADING_POINT;
dword CLOCK_F = DEFAULT_CLOCK_FREQUENCY;
FILE *output;
bool enable_exts = false;

void print_screen ()
{
    int k=0;
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (int j = 0; j < SCREEN_WIDTH; j++, k++)
        {
            if (screen[k] > 0x20)
                printf ("\033[46m%c\033[0m", screen[k]);
            else
                printf("\033[46m \033[0m");
        }
        printf ("\n");
    }
}

off_t fsize(const char *filename)
{
    struct stat st;
    
    if (stat(filename, &st) == 0)
        return st.st_size;
    
    return -1;
}

bool parse_args (int argc, const char **argv)
{
    for (int i = 2; i < argc; i++)
    {
        if (strcmp (argv[i], "-lx") == 0 && i < argc - 1)
        {
            i++;
            sscanf (argv[i], "%hx", &LOADING_POINT);
        }
        else if (strcmp (argv[i], "-l") == 0 && i < argc - 1)
        {
            i++;
            sscanf (argv[i], "%hd", &LOADING_POINT);
        }
        else if (strcmp (argv[i], "-ee") == 0) //enable-extensions
        {
            enable_exts = true;
        }
        else if (strcmp (argv[i], "-clock") == 0 && i < argc - 1)
        {
            i++;
            sscanf (argv[i], "%ud", &CLOCK_F);
            if (CLOCK_F == 0)
            {
                fprintf (stderr, "Error: clock can't be zero\n");
                return false;
            }
        }
        else
        {
            fprintf (stderr, "Error: unknown argument: '%s'\n", argv[i]);
            return false;
        }
    }
    return true;
}

int main (int argc, const char **argv)
{
    if (argc < 2)
    {
        printf ("Usage: ./6502emu <file> <options>.\n"
                "Options:\n"
                "-l(x) <loading point> [Where code will be loaded in RAM]\n"
                "-v                    [Verbose]\n");
        return 0;
    }
    if (argc > 2)
        if (!parse_args (argc, argv))
            return 0;
    
    
    struct cpu *cpu = (struct cpu*) malloc (sizeof (cpu));
    /* Set up screen */
    /*for(int i=0;i<25;i++)
        for(int j=0;j<80;j++)
            screen[i][j]=0;*/
    
    FILE *fin; /* Input file */
    fin = fopen (argv[1], "rb");
    if (!fin)
    {
        fprintf (stderr, "6502emu: Unable to open file '%s'.\n", argv[1]);
        return -1;
    }
    
    memset (RAM, 0, RAM_SIZE);
    
    /* Getting number of bytes in the file */
    fseek (fin, 0L, SEEK_END);
    cpu->size = (int) ftell (fin);
    fseek (fin , 0L, SEEK_SET);
    fread (&RAM[0]+LOADING_POINT, 1, cpu->size, fin);
    fclose (fin);
    
    printf ("\n\nFile size: \033[46m%d\033[0m(bytes).\n", cpu->size);
    printf ("Loading point: \033[46m0x%x\033[0m.\n", LOADING_POINT);
    //printf ("Ending point: \033[46m0x%x\033[0m.\n\n\n", LOADING_POINT + cpu->size);
    
    cpu_reset (cpu);
    cpu_execute (cpu);
    
    /* DEBUG */
    word addr;
    while (1)
    {
        printf ("RAM address: ");
        scanf  ("%x", (unsigned int*)&addr);
        printf ("RAM[%04x] = 0x%02x\n--------------\n", addr, RAM[addr]);
    }
    return 0;
}

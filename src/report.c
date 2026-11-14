/***************************************************************
 * report.c: the figures of a run, written to the console's line.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#include <nice.h>
#include "report.h"

static unsigned long long start_cycles, start_instructions, start_micros;

static void put_str(const char *s)
{
    while (*s)
        nice_poke(NICE_CONSOLE, (unsigned char) *s++);
}

static void put_dec(unsigned long long v)
{
    char buf[21];
    char *p = buf + sizeof buf;

    *--p = 0;
    do
    {
        *--p = (char) ('0' + v % 10);
        v /= 10;
    } while (v);
    put_str(p);
}

static void put_figure(const char *name, unsigned long long v)
{
    put_str(name);
    put_dec(v);
    put_str("\n");
}

void report_start(void)
{
    start_cycles = nice_cycles();
    start_instructions = nice_instructions();
    start_micros = nice_micros();
}

void report_end(const char *title, unsigned long iterations)
{
    unsigned long long cycles = nice_cycles() - start_cycles;
    unsigned long long instructions = nice_instructions() - start_instructions;
    unsigned long long micros = nice_micros() - start_micros;

    put_str(title);
    put_str("\n");
    put_figure("iterations   ", iterations);
    put_figure("cycles       ", cycles);
    put_figure("instructions ", instructions);
    put_figure("microseconds ", micros);
}

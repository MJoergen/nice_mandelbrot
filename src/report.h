/***************************************************************
 * report.h: the figures of a run, written to the console's line.
 *
 * The figures go straight to the console's register, not through
 * nice_putc, so that under the monitor on the MEGA65 they go to the
 * line and not over the picture. For NICE only.
 *
 * Copyright (c) 2026 Michael Jørgensen. MIT license (see LICENSE).
 ***************************************************************/

#ifndef REPORT_H
#define REPORT_H

// Takes the counters at the start of the picture.
void report_start(void);

// Writes the title, the iterations, and the clock cycles, the
// instructions, and the microseconds since report_start.
void report_end(const char *title, unsigned long iterations);

// Writes a line of text.
void report_line(const char *s);

#endif

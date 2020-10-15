/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include <stdio.h>

int fputc(int ch, FILE *f)
{
    extern void print_char(char);

    if (ch == '\n') {
        print_char('\r');
    }

    print_char(ch);

    return ch;
}

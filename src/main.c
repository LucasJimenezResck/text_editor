#include "../include/kilo.h"
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

int main(void)
{
    enableRawMode();
    char c;
    //Reads text from the terminal until user inputs 'q', any text after q
    //is not read.
    while(read(STDIN_FILENO, &c, 1) == 1 & c != 'q')
    {
        if(iscntrl(c)) //if c is a control character (nonprintable)
        {
            printf("%d\r\n", c);
        }
        else
        {
            printf("%d, ('%c')\r\n", c, c); //ASCII code + char
        }
    }
    return 0;
}
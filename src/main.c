#include "../include/kilo.h"
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

int main(void)
{
    enableRawMode();
    initEditor();
    char c;
    //Reads text from the terminal until user inputs 'q', any text after q
    //is not read. (read(STDIN_FILENO, &c, 1) == 1 & c != 'q')
    while(1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
        editorOpen();
#if 0 //Original input which returned values into the terminal
        c = '\0';
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
            die("read");

        if(iscntrl(c)) //if c is a control character (nonprintable)
        {
            printf("%d\r\n", c);
        }
        else
        {
            printf("%d, ('%c')\r\n", c, c); //ASCII code + char
        }
        //Program exits with ctrl. q
        if(c==CTRL_KEY('q'))
            break;
#endif
    }
    return 0;
}
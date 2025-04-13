#include "../include/kilo.h"

#include <ctype.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char* argv[])
{
    enableRawMode();
    initEditor();
    //If we pass to the executable a .txt-file as input, the text is stored on
    //the first line of the terminal
    if(argc >= 2)
        editorOpen(argv[1]);
    //Reads text from the terminal until user inputs 'q', any text after q
    //is not read. (read(STDIN_FILENO, &c, 1) == 1 & c != 'q')
    //Explain important commands for the editor
    editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");
    while(1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
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
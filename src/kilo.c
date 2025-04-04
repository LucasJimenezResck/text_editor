#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "../include/kilo.h"
#include <errno.h>

struct termios orig_termios;

void editorRefreshScreen()
{
    //Writes four bytes into the terminal
    //0x1b is the ESC key, followed by the [ generates an escape sequence
    //In this case it clears the whole screen, but sets the cursor to the bottom
    write(STDOUT_FILENO, "\x1b[2J", 4);
    //The H command repositions the cursor
    write(STDOUT_FILENO, "\x1b[H", 3);
}

//Editor function waits to read input from the terminal in case it's not invalid
char editorReadKey()
{
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}
//Process input coming from read key, for now doesn't do anything, just exits with Ctrl Q
void editorProcessKeypress()
{
    char c = editorReadKey();
    switch(c)
    {
        case CTRL_KEY('q'):
            exit(0);
            break;
    }
}


//Prints error message and exits the program
void die(const char* s)
{
    perror(s);
    exit(1);
}

//Disables the turned off echo after running main code so text can appear normally again
void disableRawMode()
{
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");

}

//Turns off echo
void enableRawMode()
{
    //Reads terminal attributes and calls die in case of failure (-1)
    //Examples of failure "echo test | ./text_editor"
    //"./text_editor < ../src/main.c"
    //These cause a failure because they assign files as input for the executable,
    //instead of the terminal
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");
    
    //Ensures that the original terminal configuration is set at program exit
    atexit(disableRawMode);
    
    struct termios raw = orig_termios;
    tcgetattr(STDIN_FILENO, &raw);

    //Reverts ECHO and then ANDs with a 0 on the corresponding local flags to disable them
    //Disabling ICANON exits program as soon as 'q' is even pressed
    //Disabling ISIG makes Ctrl. + C not exit the program
    //Disabling IXON takes care of Ctrl. + S and  + Q
    //Disabling IEXTEN takes care of Ctrl. + V
    //Disabling ICRNL (carriage return new line) changes the output of Ctrl. + M to 13
    //Disabling OPOST shuts down the post processing of output, like carriage return
    //Disbaling BRKINT disables the break condition, similar to Ctrl. + C
    //INPCK is for parity checking
    //Disabling ISTRIP hinders the 8th bit to be set to 0 
    //CS8 is a bit mask which sets chars to 8 bits
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //local flags
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);

    raw.c_cc[VMIN] = 0; //Min values of input for read -> 0
    raw.c_cc[VTIME] = 1; //Waits .1 seconds before read returns
    //Applies new terminal values by changing the raw variable
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}


//#include <stdio.h> //not yet used in the code
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disablRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disablRawMode);
    
    struct termios raw = orig_termios;
    tcgetattr(STDIN_FILENO, &raw);

    raw.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(void)
{
    enableRawMode();
    char c;
    //Reads text from the terminal until user inputs 'q', any text after q
    //is not read.
    while(read(STDIN_FILENO, &c, 1) == 1 & c != 'q');
    //printf("Hello Viki!\n");
    //printf("Environment successfully installed in WSL!\n");
    return 0;
}
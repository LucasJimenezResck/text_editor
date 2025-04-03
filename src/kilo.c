#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

//Disables the turned off echo after running main code so text can appear normally again
void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

//Turns off echo
void enableRawMode()
{
    //Reads terminal attributes
    tcgetattr(STDIN_FILENO, &orig_termios);
    
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
    //Applies new terminal values by changing the raw variable
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

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
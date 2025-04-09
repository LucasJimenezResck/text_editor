#include "../include/kilo.h"

struct editorConfig E;

/*** output ***/
//Checks if cursor has moved outside visible window, and adjust the window in that case
void editorScroll()
{
    //If cursor above the read window, scroll up to where the cursor is
    if(E.cy < E.rowoff)
        E.rowoff = E.cy;
    //To address what is below we use the screenrows attribute
    if(E.cy >= E.rowoff + E.screenrows)
        E.rowoff = E.cy - E.screenrows + 1;
}

void editorDrawRows(struct abuf* ab)
{
    int y;
    for(y=0; y<E.screenrows;y++)
    {
        int filerow = y + E.rowoff; //We use the row value plus the scroll-offset
        if(filerow >= E.numrows) //Are we drawing a part of the buffer or is it after it?
        {
            //At about a third of the screen print out a welcome message
            if(y==E.screenrows / 3 && E.numrows == 0) //If buffer is empty, display welcome
            {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), 
                "Kilo editor -- version %s", KILO_VERSION);
                if(welcomelen > E.screencols)
                    welcomelen = E.screencols;
                //Centers the screen to exactly the start of the message
                int padding = (E.screencols - welcomelen) / 2;
                //Starting character is a ~
                if(padding)
                {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                //Rest is just blank
                while(padding--)
                    abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen); 
            }
            //Draws a column of tildes on the left of the screen
            //Handles each row of text being edited
            //Static input: write(STDOUT_FILENO, "~", 1);
            //All instances of abAppend in code were first like the write command
            else
            {
                abAppend(ab, "~", 1);
            }
        }
        else
        {
            int len = E.row[filerow].size;
            if(len > E.screencols)
                len = E.screencols;
            abAppend(ab, E.row[filerow].chars, len);
        }
        abAppend(ab,"\x1b[K", 3);
        //Makes sure last line is an exception to drawing a new line so that all lines have ~
        if(y < E.screenrows - 1)
        {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen()
{
    editorScroll();
    struct abuf ab = ABUF_INIT;

    
    //This line corrects the flickering of the cursor during while the terminal is drawing
    //It hides and then reappears the cursor, some terminals may ignore the command
    abAppend(&ab, "\x1b[?25l", 6);
    //Writes four bytes into the terminal
    //0x1b is the ESC key, followed by the [ generates an escape sequence
    //In this case it clears the whole screen, but sets the cursor to the bottom
    //Later replaced for another command while drawing the rows
        //abAppend(&ab,"\x1b[2J", 4);
    //The H command repositions the cursor to the top and can take two arguments
    //for row and column like [12;40H, default is 1;1
    abAppend(&ab, "\x1b[H", 3);
    editorDrawRows(&ab);

    //H command with arguments to specify exactly where to move the cursor, adjusted for offset
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    //All of buffer's contents are written out and memory is set free
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*** input ***/
//Process input coming from read key, for now doesn't do anything, just exits with Ctrl Q
void editorProcessKeypress()
{
    int c = editorReadKey();
    switch(c)
    {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            E.cx = E.screencols - 1;
            break;
        //Move the cursor up or down until reached the limit of the screen
        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                while(times--)
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

/*** terminal ***/
//Prints error message and exits the program
void die(const char* s)
{
    //Both write commands added at exit to clear the screen instead of atexit() to
    //keep the error message
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

//Disables the turned off echo after running main code so text can appear normally again
void disableRawMode()
{
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
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
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");
    
    //Ensures that the original terminal configuration is set at program exit
    atexit(disableRawMode);
    
    struct termios raw = E.orig_termios;
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

//Editor function waits to read input from the terminal in case it's not invalid
int editorReadKey()
{
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(nread == -1 && errno != EAGAIN)
            die("read");
    }

    //If we read an esc byte, we go to read three bytes that code the arrow keys
    if(c == '\x1b')
    {
        char seq[3];
        //If not respond, we assume just esc was pressed
        if(read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';
        
        //Code the command for the arrow keys, it now can store three bytes
        if(seq[0] == '[')
        {
            //Check if second byte is a digit
            if(seq[1] >= '0' && seq[1] <= '9')
            {
                if(read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if(seq[2] == '~')
                {
                    switch(seq[1])
                    {
                        case '1':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                        case '7':
                            return HOME_KEY;
                        case '8':
                            return END_KEY;
                    }
                }
            }
            else
            {
                switch(seq[1])
                {
                    case 'A':
                        return ARROW_UP;
                    case 'B':
                        return ARROW_DOWN;
                    case 'C':
                        return ARROW_RIGHT;
                    case 'D':
                        return ARROW_LEFT;
                    case 'H':
                        return HOME_KEY;
                    case 'F':
                        return END_KEY;
                }
            }
        }
        else if(seq[0] == 'O')
        {
            switch(seq[1])
            {
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        }
        //If unknown, just return esc
        return '\x1b';
    }
    else 
    {
        return c;
    }
}

int getCursorPosition(int* rows, int* cols)
{
    char buf[32];
    unsigned int i = 0;
    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;
    
    while(i < sizeof(buf) - 1)
    {
        if(read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if(buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';
    //If first element of buffer isn't an esc commando followed by a [ triggers failure
    if(buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    //If coordinates of window size aren't in the correct format or enough info
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;
    return 0;
}

int getWindowSize(int* rows, int* cols)
{
    struct winsize ws; //failure by ioctl or dimensions gives back error value
    //TIOCGWINSZ: Terminal I/O Control get Window Size
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        //Makes sure to request window size on more systems
        //1. Moves cursor forward and then down, C and B commands stop when finding the edge
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(rows, cols);
    }
    else //Success: dimensions of given window size stored into the variables
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void editorMoveCursor(int key)
{
    switch(key)
    {
        case ARROW_UP:
            if(E.cy != 0)
                E.cy--;
            break;
        case ARROW_LEFT:
            if(E.cx != 0)
                E.cx--;
            break;
        //Can advance past the bottom of the screen, but not the bottom of the file
        case ARROW_DOWN:
            if(E.cy < E.numrows)
                E.cy++;
            break;
        case ARROW_RIGHT:
            if(E.cx != E.screencols - 1)
                E.cx++;
            break;
        default:
            break;
    }
}
/*** row operations ***/

void editorAppendRow(char* s, size_t len)
{
    //Allocate the number of bytes each erow takes times the rows we want to print
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    int at = E.numrows;
    E.row[at].size = len;
    
    //Allocate one more space than the length to include the end of string
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len); //copy the message with the defined length to erow
    E.row[at].chars[len] = '\0';
    E.numrows++;
}


/*** file i/o ***/
void editorOpen(char* filename)
{

    FILE *fp = fopen(filename, "r");
    if(!fp)
        die("fopen");

    //Stores a line and allocates memory for the message
    char* line = NULL;
    ssize_t linelen;
    ssize_t linecap = 0;

    //allocate memory for new line in linelen until there are no more lines to read
    while((linelen = getline(&line, &linecap, fp)) != -1)
    {
        while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorAppendRow(line, linelen);
    }
    free(line); //Allocated line is set free
    fclose(fp);
}

/*** append buffer ***/

void abAppend(struct abuf* ab, const char* s, int len)
{
    //Allocate memory big enough to hold current string length plus new text
    //Either extends size of current block or sets it free and reallocates to new block
    char *new = realloc(ab->b, ab->len + len);
    //No block found, stop operation
    if(new == NULL)
        return;
    //Copies new string to end of text and updates values of abuf 
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

//"Destructor" which sets allocated memory free
void abFree(struct abuf* ab)
{
    free(ab->b);
}
/*** init ***/
//Initializes all fields on E-structure
void initEditor()
{
    E.cx = 0;
    E.cy = 0;
    E.numrows = 0;
    E.rowoff = 0; //Default value is top of the screen
    E.row = NULL;
    if(getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
}
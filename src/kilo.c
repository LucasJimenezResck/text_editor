#include "../include/kilo.h"


struct editorConfig E;

//Highlight database
char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL };
char *C_HL_Keywords[] = {
    //Primary keywords
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "class", "case",
    //Secondary keywords
    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", NULL
  };
struct editorSyntax HLDB[] = {
  {
    "c",
    .filematch = C_HL_extensions,
    .keywords = C_HL_Keywords,
    .singleline_comment_start = "//",
    .multiline_comment_start = "/*",
    .multiline_comment_end = "*/",
    .flags = HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
};
//Define length of the array
#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))



/*** output ***/
//Checks if cursor has moved outside visible window, and adjust the window in that case
void editorScroll()
{
    E.rx = 0;
    if(E.cy < E.numrows)
        E.rx = editorRowRxToCx(&E.row[E.cy], E.cx);
    //If cursor above the read window, scroll up to where the cursor is
    if(E.cy < E.rowoff)
        E.rowoff = E.cy;
    //To address what is below we use the screenrows attribute
    if(E.cy >= E.rowoff + E.screenrows)
        E.rowoff = E.cy - E.screenrows + 1;
    //Analogue to the vertical conditions 
    if(E.rx < E.coloff)
        E.coloff = E.rx;
    if(E.rx >= E.coloff + E.screencols)
        E.coloff = E.rx - E.screencols + 1;
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
            int len = E.row[filerow].rsize - E.coloff;
            if(len < 0) //Avoid user from scrolling past the end of the line
                len = 0;
            if(len > E.screencols)
                len = E.screencols;
            char* c = &E.row[filerow].render[E.coloff];
            //slice of hl corresponding to the slice being printed
            unsigned char* hl = &E.row[filerow].hl[E.coloff];
            int current_color = -1;
            int j;
            //Feed the substring of render char by char
            for(j = 0; j < len; j++)
            {
                //If character is ctrl. type
                if(iscntrl(c[j]))
                {
                    //Define symbol between @ and Z or non-alphabetical
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    //Invert colors for these values and then put back to normal
                    abAppend(ab, "\x1b[7m", 4);
                    abAppend(ab, &sym, 1);
                    abAppend(ab, "\x1b[m", 3);
                    //Reverse the turned off formating done by the former block
                    if(current_color != -1)
                    {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        abAppend(ab, buf, clen);
                    }
                }
                
                //If it is not a digit
                else if(hl[j] == HL_NORMAL)
                {
                    //Make sure to be using the normal text to print
                    //Same style was used to show the status bar
                    if(current_color != -1)
                    {
                        abAppend(ab, "\x1b[39m", 5);
                        current_color = -1;
                    }
                    abAppend(ab, &c[j], 1);
                }
                else
                {
                    int color = editorSyntaxToColor(hl[j]);
                    //Just print out an escape sequence if the color changes
                    if(color != current_color)
                    {
                        current_color = color;
                        char buf[16];
                        //Write an escape sequence for the numbers using the syntax to color
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        abAppend(ab, buf, clen);
                    }
                        abAppend(ab, &c[j], 1);
                    
                }
            }
            //At the end, make sure we are at default
            abAppend(ab, "\x1b[39m", 5);
        }
        abAppend(ab,"\x1b[K", 3);
        //Makes sure last line is an exception to drawing a new line so that all lines have ~
        
        abAppend(ab, "\r\n", 2);
        
    }
}
void editorDrawStatusBar(struct abuf *ab)
{
    //m makes the text to be printed with various attributes: \x1b[1;4;5;7m
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    //Prints out a bar showing the number of lines and text name at the bottom of the screen
    //Also if the file was changed after opened or saving
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    E.filename ? E.filename : "[No Name]", E.numrows,
    E.dirty ? "(modified)" : "");
    //Displays the number of the current line, filetype and the total lines on the right of the bar
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
    E.syntax ? E.syntax->filetype : "no ft", E.cy + 1, E.numrows);
    if(len > E.screencols)
        len = E.screencols;
    abAppend(ab, status, len);
    while(len < E.screencols)
    {
        //Keep printing status until it would end up at the right edge of the screen
        if(E.screencols - len == rlen)
        {
            abAppend(ab, rstatus, rlen);
            break;
        }
        else
        {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}
void editorDrawMessageBar(struct abuf* ab)
{
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if(msglen > E.screencols)
        msglen = E.screencols;
    //Message displays the text for only 5 seconds
    if(msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
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
    //Display text rows plus messages below
    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    //H command with arguments to specify exactly where to move the cursor, adjusted for offset
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    //All of buffer's contents are written out and memory is set free
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
//Variadic function, all va_ elements are there because of that
void editorSetStatusMessage(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL); //Time in UNIX timestamp
}

/*** input ***/

char* editorPrompt(char* prompt, void (*callback)(char*, int))
{
    size_t bufsize = 128;
    //Dynamically allocate the user's input in buf
    char* buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';
    while(1)
    {
        //Set status message, refresh the screen and wait for input to handle
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();
        int c = editorReadKey();
        //If a delete or backspace key was pressed, delete last character
        if(c == DEL_KEY || c== CTRL_KEY('h') || c == BACKSPACE)
        {
            if(buflen != 0)
                buf[--buflen] = '\0';
        }
        //When pressed escape free the allocated memory and avoid the 
        else if(c == '\x1b')
        {
            editorSetStatusMessage("");
            if(callback)
                callback(buf, c);
            free(buf);
            return NULL;
        }
        //When pressed enter and the message is not empty return the buffer with the name
        else if(c == '\r')
        {
            if(buflen != 0)
            {
                editorSetStatusMessage("");
                //Allows the user to pass NULL in case callback isn't wanted
                if(callback)
                    callback(buf, c);
                return buf;
            }
        }
        //Printable character gets appended to buf
        else if(!iscntrl(c) && c < 128)
        {
            //If reached the maximum capacity, double the bufsize and allocate it
            if(buflen == bufsize - 1)
            {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            //Make sure the last char is a 0
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
        if(callback)
            callback(buf, c);
    }
}
//Process input coming from read key, for now doesn't do anything, just exits with Ctrl Q
void editorProcessKeypress()
{
    static int quit_times = KILO_QUIT_TIMES;
    int c = editorReadKey();
    switch(c)
    {
        case '\r':
            editorInsertNewLine();
            break;
        case CTRL_KEY('q'):
            if(E.dirty && quit_times > 0)
            {
                editorSetStatusMessage("WARNING: file has unsaved changes. "
                "Press Ctrl-Q %d more times to exit", quit_times);
                quit_times--;
                return; //goes back to the switch case
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        //Save commited changes in the file
        case CTRL_KEY('s'):
            editorSave();
            break;
        case HOME_KEY:
            E.cx = 0;
            break;
        //Bring cursor to the end of the line unles we are at 0
        case END_KEY:
            if(E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;
        //Find certain words into the text
        case CTRL_KEY('f'):
            editorFind();

        case BACKSPACE:
        case CTRL_KEY('h'):
        //For the delete key move the cursor to the other side
        case DEL_KEY:
            if(c == DEL_KEY)
                editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;
        //Move the cursor up or down until reached the limit of the screen
        case PAGE_UP:
        case PAGE_DOWN:
            {
                if(c == PAGE_UP)
                {
                    E.cy = E.rowoff;
                }
                else if(c == PAGE_DOWN)
                {
                    E.cy = E.rowoff + E.screenrows - 1;
                    if(E.cy > E.numrows)
                        E.cy = E.numrows;
                }
            }
            break;
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
        //Ctrl L to refresh and esc key don't do anything
        case CTRL_KEY('l'):
        case '\x1b':
            break;
        //Turns text viewer into an editor
        default:
            editorInsertChar(c);
            break;
    }
    //If user doesn't press quit three times in a row, counter is reset
    quit_times = KILO_QUIT_TIMES;
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
    //Check if the cursor is on an actual line
    erow* row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    switch(key)
    {
        case ARROW_UP:
            if(E.cy != 0)
                E.cy--;
            break;
        case ARROW_LEFT:
            if(E.cx != 0)
                E.cx--;
        //If when typing left there is a line above, we move to the end of the previous one
            else if(E.cy > 0)
            {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        //Can advance past the bottom of the screen, but not the bottom of the file
        case ARROW_DOWN:
            if(E.cy < E.numrows)
                E.cy++;
            break;
        case ARROW_RIGHT:
        //If E.cx is to the left of the end, then it can move right
            if(row && E.cx < row->size)
                E.cx++;
        //If we are not at the end of the file, moving further right brings to the start below
            else if(row && E.cx == row->size)
            {
                E.cy++;
                E.cx = 0;
            }
            break;
        default:
            break;
    }
    //Generates the snapback so that when scrolling the cursor stays at the end of the line
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if(E.cx > rowlen)
        E.cx = rowlen;
}
/*** syntax highlight ***/
//Return if the character can be considered as separator
int is_separator(int c)
{
    //strchr returns the first pointer matching a character in the given string
    //If it doesn't find it, it returns null
    return isspace(c) || c=='\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}
//Goes through values of an erow and highlights its numbers
void editorUpdateSyntax(erow* row)
{
    //Allocate the needed space (a row)
    row->hl = realloc(row->hl, row->rsize);
    //Initialy set all characters to normal
    memset(row->hl, HL_NORMAL, row->rsize);
    //Take filetype into account for highlighting, no c-file means no highlighting
    if(E.syntax == NULL)
        return;
    char **keywords = E.syntax->keywords;
    //Store the pointer to the char for comment start and the given length, otherwise no length
    //Also for multiline comment
    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;
    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;
    //We consider the beginning of the line to be a separator
    int prev_sep = 1;
    //Keep track of whether we are inside a string or a comment
    int in_string = 0;
    //In comment is true if positive index and former line contains an open ml comment
    int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);
    int i = 0;
    //Iterate through the characters and set the corresponding values to number
    while(i < row->rsize)
    {
        char c = row->render[i];
        //Highlight type of the previous number
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
        //Make sure of the length, if we're not in a string and not inside an ml comment
        if(scs_len && !in_string && !in_comment)
        {
            //Check if char is the start of a scs
            if(!strncmp(&row->render[i], scs, scs_len))
            {
                //Set the rest of the line to comment and break out of the loop
                memset(&row->hl[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }
        //If our comments aren't null and we aren't inside of a string
        if(mcs_len && mce_len && !in_string)
        {
            if(in_comment)
            {
                row->hl[i] = HL_MLCOMMENT;
                //If we are at the end of a comment set to highlighted type and get out of comment bool
                if(!strncmp(&row->render[i], mce, mce_len))
                {
                    memset(&row->hl[i], HL_MLCOMMENT, row->rsize - i);
                    //Consume comment type
                    i+= mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                }
                //Just consume the char if it's not the end
                else
                {
                    i++;
                    continue;
                }
            }
            //If we aren't at an ml comment check if we are at the beginning of it and set the
            //boolean
            else if(!strncmp(&row->render[i], mcs, mcs_len))
            {
                memset(&row->hl[i], HL_MLCOMMENT, row->rsize - i);
                i+= mcs_len;
                in_comment = 1;
                continue;
            }
        }
        if(E.syntax->flags & HL_HIGHLIGHT_STRINGS)
        {
            if(in_string)
            {
                //Set the highlighter to string type
                row->hl[i] = HL_STRING;
                //Consider also escape quotes
                if(c=='\\' && i + 1 < row->size)
                {
                    //Highlight the char after the backslash and consume both of them
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                //If the char is a closing quote stop the in string
                if(c == in_string)
                    in_string = 0;
                i++;
                //Closing quote is considered a separator
                prev_sep = 1;
                continue;
            }
            else
            {
                //Store for single and double quote
                if(c == '"' || c == '\'')
                {
                    //Set the value of c to in string in case it is the beginning or end of a strings
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }
        if(E.syntax->flags & HL_HIGHLIGHT_NUMBERS)
        {
            //To highlight a character, now the previous character must be either a separator
            //Or a number as well
            //Decimal points are also considered part of the number
            if(isdigit(c) && (prev_sep || prev_hl == HL_NUMBER) || (c == '.' && prev_hl && HL_NUMBER))
            {
                
                row->hl[i] = HL_NUMBER;
                i++;
                //Indicates we are highlighting the number and continue with the loop
                prev_sep = 0;
                continue;
            }
        }
        //Make sure there is a separator before the keyword
        if(prev_sep)
        {
            int j;
            for(j = 0; keywords[j]; j++)
            {
                //Find keyword length for both types of kw
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                //For secondary remove last char
                if(kw2)
                    klen--;
                
                //Check if the keyword exists and if it is followed by a separator
                if(!strncmp(&row->render[i], keywords[j], klen) &&
                is_separator(row->render[i + klen]))
                {
                    //Set the highlighter to the corresponding kw type and consume it

                    memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    //Break out of the inner loop before continuing
                    break;
                }
            }
            //Check if the loop was actually broken out of
            if(keywords[j] != NULL)
            {
                prev_sep = 0;
                continue;
            }
        }
        //If we don't highlight the character, set previous separator as current char and consume it
        prev_sep = is_separator(c);
        i++;
    }
    //Set the open comment state to the last value of in comment
    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    //Updates syntax for the following lines in case an ml comment was made
    if(changed && row->idx + 1 < E.numrows)
        editorUpdateSyntax(&E.row[row->idx + 1]);
}
//If it's a number return the red code, otherwise return white
int editorSyntaxToColor(int hl)
{
    switch(hl)
    {
        case HL_NUMBER:
            return 31;
        case HL_KEYWORD1:
            return 32;
        case HL_KEYWORD2:
            return 33;
        case HL_MATCH:
            return 34;
        case HL_STRING:
            return 35;
        case HL_MLCOMMENT:
        case HL_COMMENT:
            return 36;
        default:
            return 37;
    }
}
void editorSelectSyntaxHighlight()
{
    E.syntax = NULL;
    //No filename means no highlighting
    if(E.filename == NULL)
        return;
    //Find the last occurrence of the extension part of the filename
    char* ext = strrchr(E.filename, '.');
    for(unsigned int j = 0; j < HLDB_ENTRIES; j++)
    {
        //Loop through elements in highlight database
        struct editorSyntax* s = &HLDB[j];
        unsigned int i = 0;
        //Loop through each filematch
        while(s->filematch[i])
        {
            //If the pattern starts with . it is an extension
            int is_ext = (s->filematch[i][0] == '.');
            if((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
            (!is_ext && strstr(E.filename, s->filematch[i])))
            {
                E.syntax = s;
                int filerow;
                for(filerow = 0; filerow < E.numrows; filerow++)
                {
                    //Loop through every file line and update the syntax so that the
                    //highlighting applies for new files
                    editorUpdateSyntax(&E.row[filerow]);
                }
                return;
            }
            i++;
        }
    }
}
/*** row operations ***/
//Uses the chars of erow to fill the contents of render

int editorRowRxToCx(erow* row, int rx)
{
    int cur_rx = 0;
    int cx;
    //loop through the chars calculating the current rx
    for(cx = 0; cx < row->size; cx++)
    {
        if(row->chars[cx] == '\t')
        //If it is a tab subtract right distance from last tab to left distance
            cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
        
        cur_rx++;
        //Stop when found the current rx and return cx
        if(cur_rx > rx)
            return cx;
    }
    //In case we find something out of range
    return cur_rx;
}

void editorUpdateRow(erow* row)
{
    int tabs = 0;
    int j;
    for(j = 0; j < row->size; j++)
    {
        if(row->chars[j] == '\t') //Iterate through the text to know the amount of tabs
            tabs++;
    }
    free(row->render);
    //allocate 8 spaces for a tab, one is already stored in size
    row->render = malloc(row->size + tabs * (KILO_TAB_STOP - 1) + 1); 

    int idx = 0;
    for(j = 0; j < row->size; j++)
    {
        //When encountered with a tab, create 8 blank spaces, tab length is now constant
        if(row->chars[j] == '\t')
        {
            row->render[idx++] = ' ';
            while(idx % KILO_TAB_STOP != 0)
                row->render[idx++] = ' ';
        }
        else
        {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0'; //contains values of chars
    row->rsize = idx; //contains number of characters assigned to render
    editorUpdateSyntax(row); // Highlight the numbers
}

void editorInsertRow(int at, char* s, size_t len)
{
    //Validate at
    if(at < 0 || at > E.numrows)
        return;
    //Allocate the number of bytes each erow takes times the rows we want to print
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    //Make room for the new row
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

    for(int j = at + 1; j <= E.numrows; j++)
        E.row[j].idx++;
    
    E.row[at].idx = at;
    E.row[at].size = len;
    
    //Allocate one more space than the length to include the end of string
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len); //copy the message with the defined length to erow
    E.row[at].chars[len] = '\0';
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    E.row[at].hl = NULL;
    E.row[at].hl_open_comment = 0;
    editorUpdateRow(&E.row[at]);
    E.numrows++;
    E.dirty++; //Flag is set for every operation that alters text (could have been set to 1)
}

void editorFreeRow(erow* row)
{
    free(row->chars);
    free(row->render);
    free(row->hl);
}

void editorDelRow(int at)
{
    //Validate at
    if(at < 0 || at >= E.numrows)
        return;
    //Free memory used by row
    editorFreeRow(&E.row[at]);
    //Overwrite deleted row with the ones that come after it, update flag and total rows
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    for(int j = at + 1; j <= E.numrows; j++)
        E.row[j].idx--;
    E.dirty++;
    E.numrows--;
}

void editorRowInsertChar(erow* row, int at, int c)
{
    //Validate the index to put the character into
    if(at < 0 || at > row->size)
        at = row->size;
    //Reallocate one more space to do over the line and for the 0
    row->chars = realloc(row->chars, row->size + 2);
    //Assign the character to the position in array and update row
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++; //Flag is set for every operation that alters text
}
void editorRowAppendString(erow* row, char* s, size_t len)
{
    //The row's new size is equal to the current size, plus the length of the string and the 0
    row->chars = realloc(row->chars, row->size + len + 1);
    //Copy the given string to the end of row->chars
    memcpy(&row->chars[row->size], s, len);
    //Update size, add the null, update the row and set the flag
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++; 
}

void editorRowDelChar(erow* row, int at)
{
    if(at < 0 || at >= row->size)
        return;
    //Memory move opposite to row insert char
    //Overwrite deleted character with the one that comes after it
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    //Row size decremented and change notified
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

/*** editor operations ***/
void editorInsertChar(int c)
{
    //If cursor is after the end of the file, append a row
    if(E.cy == E.numrows)
        editorInsertRow(E.numrows, "", 0);
    //Insert a character and move the cursor forward
    //Without worrying about the details of the cursor
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewLine()
{
    //If at the beginning of a line, just insert a new blank line
    if(E.cx == 0)
    {
        editorInsertRow(E.cy, "", 0);
    }
    //Otherwise
    else
    {
        erow* row = &E.row[E.cy];
        //Split row in two
        //Pass characters on the right of the cursor to new inserted row
        //Already updated
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        //Pass again the correct values to row due to reallocation
        //Size is now the position of the cursor
        //Add an end of the line and update the truncated row
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    //In both cases, update the cursor values
    E.cy++;
    E.cx = 0;
}

void editorDelChar()
{
    //Nothing to delete if the cursor is past the end of the file
    //Or if it is at the very beginning
    if(E.cy == E.numrows)
        return;
    if(E.cx == 0 && E.cy == 0)
        return;
    //Get the cursor positions and if there is a character left to the cursor, delete it
    erow* row = &E.row[E.cy];
    if(E.cx > 0)
    {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    }
    //If cursor is not at the beginning of the file but it is at the beginning of a row
    else
    {
        E.cx = E.row[E.cy - 1].size;
        //Append row->chars to the previous row and delete the current row
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

/*** file i/o ***/

char* editorRowsToString(int* buflen)
{
    int totlen = 0;
    int j;
    for(j = 0; j < E.numrows; j++)
    //Add the length of each row into the variable plus the new line char
        totlen += E.row[j].size + 1;
    *buflen = totlen;
    
    //Allocate the required memory, loop through the rows and copy the contents to the buffer
    //Adding a new line at the end
    char* buf = malloc(totlen);
    char* p = buf;

    for(j = 0; j < E.numrows; j++)
    {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void editorOpen(char* filename)
{
    //Makes a copy of the string
    free(E.filename);
    E.filename = strdup(filename);
    editorSelectSyntaxHighlight();

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
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line); //Allocated line is set free
    fclose(fp);
    //Flag set to 0 because appendRow was called
    E.dirty = 0;
}

void editorSave()
{
    //If it's a new file, save it with a new name
    if(E.filename == NULL)
    {
        E.filename = editorPrompt("Save as: %s (ESC. to cancel)", NULL);
        if(E.filename == NULL)
        {
            editorSetStatusMessage("Save aborted!");
            return;
        }
        editorSelectSyntaxHighlight();

    }
        
    int len;
    char* buf = editorRowsToString(&len);

    //Open a file for read and write, and create it if it doesn't exist
    //Grant the necessary permissions for text files
    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    //Set the file to be the exact size of the specified length
    //ftruncate is also called between the read and write to save most of the data in
    //the case of a failure.
    //If-blocks are there to free the allocated memory even in failure by open, truncate or write
    if(fd != -1)
    {
        if(ftruncate(fd, len) != -1)
        {
            if(write(fd, buf, len) == len)
            {
                close(fd);
                free(buf);
                //notify the user about the saving, set flag down
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }
    //Notify in case of failure in save
    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));

}
/*** info ***/

void editorFindCallback(char* query, int key)
{
    //Static variables to contain where the last match was on, and whether to keep looking
    //forwards or backwards
    static int last_match = -1;
    static int direction = 1;
    //Make sure results don't stay highlighted after search
    //Know which lines to restore and copy them to the saved line's highlight pointer
    static int saved_hl_line;
    static char* saved_hl = NULL;
    if(saved_hl)
    {
        //Line used as index because the file can't be edited between saving and restoring hl
        memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
        //Free at the top impedes malloc() to be used without an available pointer
        free(saved_hl);
        saved_hl = NULL;
    }

    
    //If pressed escape or enter, the prompt is terminated and we return to the initial
    //condition of the variables
    if(key == '\x1b' || key == '\r'){
        last_match = -1;
        direction = 1;
        return;
    }
    else if(key == ARROW_RIGHT || key == ARROW_DOWN)
    {
        direction = 1;
    }
    else if(key == ARROW_LEFT || key == ARROW_UP)
    {
        direction = -1;
    }
    else
    {
        last_match = -1;
        direction = 1;
    }

    if(last_match == -1)
        direction = 1;
    //Index of current row being searched
    int current = last_match;
    int i;
    for(i = 0; i < E.numrows; i++)
    {
        current += direction;
        //If there was a match, start on the line after or before, if we start from below
        if(current == -1)
            current = E.numrows - 1;
        //No last match means we start from the beginning
        else if(current == E.numrows)
            current = 0;
        
        erow* row = &E.row[current];
        //Check if query is a substring of a current row
        char *match = strstr(row->render, query);
        if(match)
        {
            //Update last match for user to start searching from that point
            last_match = current;
            //Turn the matching pointer into an index
            E.cy = current;
            E.cx = editorRowRxToCx(row, match - row->render);
            E.rowoff = E.numrows;
            //Match the substring to the highlight
            saved_hl_line = current;
            saved_hl = malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void editorFind()
{
    int saved_cx = E.cx;
    int saved_cy = E.cy;
    int saved_coloff = E.coloff;
    int saved_rowoff = E.rowoff;

    char* query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)", editorFindCallback);
    if(query)
    {
        free(query);
    }
    //If user presses ESC, go back to the previous cursor position
    //Query is NULL
    else
    {
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    }
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
    E.rx = 0;
    E.cy = 0;
    E.numrows = 0;
    E.rowoff = 0; //Default value is top of the screen
    E.coloff = 0; //Default value is left of the screen
    E.dirty = 0; //Flag is not set at default
    E.row = NULL;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    E.syntax = NULL;
    if(getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
    //Reserve screenrow for status bar and message
    E.screenrows -= 2;
}
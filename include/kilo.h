#pragma once
#include <termios.h>
//Used to map Ctrl. combinations to specific terminal operations
//The bitwise operation sets the upper three bits to 0, which mirrors what the
//Ctrl. key does in the terminal
#define CTRL_KEY(k) ((k) & 0x1f)
//"Constructor" for empty buffer with length 0 and null pointer
#define ABUF_INIT {NULL, 0}
#define KILO_VERSION "0.0.1"

//Struct used to store dimension of the terminal
struct editorConfig
{
    int cx,cy; //Cursor coordinates
    int screenrows; //Screen dimensions, rows and columns
    int screencols;
    struct termios orig_termios; //Takes care of enabling and disabling certain terminal modes
};

//Dynamic string type to suppport appending operations
struct abuf
{
    char *b;
    int len;
};

void disableRawMode();
void enableRawMode();
void die(const char* s);
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows(struct abuf* ab);
int getWindowSize(int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);
void initEditor();
void abAppend(struct abuf* ab, const char* s, int len);
void abFree(struct abuf* ab);
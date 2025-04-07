#pragma once
#include <termios.h>
//Used to map Ctrl. combinations to specific terminal operations
//The bitwise operation sets the upper three bits to 0, which mirrors what the
//Ctrl. key does in the terminal
#define CTRL_KEY(k) ((k) & 0x1f)
//"Constructor" for empty buffer with length 0 and null pointer
#define ABUF_INIT {NULL, 0}
#define KILO_VERSION "0.0.1"

//Struct to store row of text in the editor
//Editor row includes the dynamically allocated data and its length
typedef struct erow
{
    int size;
    char* chars;
} erow;



//Struct used to store dimension of the terminal
struct editorConfig
{
    int cx,cy; //Cursor coordinates
    int screenrows; //Screen dimensions, rows and columns
    int screencols;
    struct termios orig_termios; //Takes care of enabling and disabling certain terminal modes
    int numrows; //Number of the row to be written
    erow row; //Takes care of the editor row
};

//Dynamic string type to suppport appending operations
struct abuf
{
    char *b;
    int len;
};

//Use of enum to describe the arrow keys makes some changes in the key reading, changing to int
//Page up and page down to move to the  vertical extremes of the screen
//For now, home and end key move to the left and right extremes
enum editorKey {ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN, DEL_KEY, HOME_KEY, END_KEY,
PAGE_UP, PAGE_DOWN};

void disableRawMode();
void enableRawMode();
void die(const char* s);
int editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows(struct abuf* ab);
int getWindowSize(int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);
void initEditor();
void abAppend(struct abuf* ab, const char* s, int len);
void abFree(struct abuf* ab);
void editorMoveCursor(int key);
void editorOpen();
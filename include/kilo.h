#pragma once
//Macros to make the code more portable, for me they weren't necessary
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

//Used to map Ctrl. combinations to specific terminal operations
//The bitwise operation sets the upper three bits to 0, which mirrors what the
//Ctrl. key does in the terminal
#define CTRL_KEY(k) ((k) & 0x1f)
//"Constructor" for empty buffer with length 0 and null pointer
#define ABUF_INIT {NULL, 0}
#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 8
//Used to confirm exit even after not saving
#define KILO_QUIT_TIMES 3


//Struct to store row of text in the editor
//Editor row includes the dynamically allocated data and its length
typedef struct erow
{
    int idx;
    int size;
    int rsize;
    char* chars;
    char* render;
    unsigned char* hl; //store the highlighting of each line
    int hl_open_comment;
} erow;

//Structure to configure highlight based on filetype being displayed
struct editorSyntax
{
    char* filetype; //diplayed filetype
    char **filematch; //array of strings to match filenames to patterns
    char **keywords; //Highlight certain specific keywords
    char* singleline_comment_start; //Which type of char consists of a single line comment
    char* multiline_comment_start;
    char* multiline_comment_end;
    int flags;
};

//Struct used to store dimension of the terminal
struct editorConfig
{
    int cx,cy; //Cursor coordinates
    int rx; //index into the render field
    int rowoff; //Row offset for scrolling
    int coloff; //Column offset
    int screenrows; //Screen dimensions, rows and columns
    int screencols;
    struct termios orig_termios; //Takes care of enabling and disabling certain terminal modes
    int numrows; //Number of the row to be written
    erow *row; //Takes care of the editor row, pointer to dynamically allocate many rows
    int dirty; //Dirty flag notifies if a file was modified since its saving or opening
    char* filename; //Save a copy of the filename displayed
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax* syntax;
};

//Dynamic string type to suppport appending operations
struct abuf
{
    char *b;
    int len;
};

//Possible values the highlight can contain
enum editorHighlight {HL_NORMAL = 0, HL_KEYWORD1, HL_KEYWORD2, HL_COMMENT, HL_MLCOMMENT, HL_STRING,
HL_NUMBER, HL_MATCH};
//Flag bits
#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

//Use of enum to describe the arrow keys makes some changes in the key reading, changing to int
//Page up and page down to move to the  vertical extremes of the screen
//For now, home and end key move to the left and right extremes
enum editorKey {BACKSPACE = 127, ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN, DEL_KEY, HOME_KEY, END_KEY,
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
void editorOpen(char* filename);
void editorInsertRow(int at, char* s, size_t len);
void editorUpdateRow(erow* row);
void editorScroll();
int editorRowRxToCx(erow* row, int rx);
void editorDrawStatusBar(struct abuf *ab);
void editorSetStatusMessage(const char* fmt, ...);
void editorDrawMessageBar(struct abuf* ab);
void editorRowInsertChar(erow* row, int at, int c);
void editorInsertChar(int c);
char* editorRowsToString(int* buflen);
void editorSave();
void editorRowDelChar(erow* row, int at);
void editorDelChar();
void editorFreeRow(erow* row);
void editorDelRow(int at);
void editorRowAppendString(erow* row, char* s, size_t len);
void editorInsertNewLine();
char* editorPrompt(char* prompt, void (*callback)(char*, int));
void editorFind();
void editorFindCallback(char* query, int key);
void editorUpdateSyntax(erow* row);
int editorSyntaxToColor(int hl);
int is_separator(int c);
void editorSelectSyntaxHighlight();
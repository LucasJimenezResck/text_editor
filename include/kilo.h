#pragma once
#include <termios.h>
//Used to map Ctrl. combinations to specific terminal operations
//The bitwise operation sets the upper three bits to 0, which mirrors what the
//Ctrl. key does in the terminal
#define CTRL_KEY(k) ((k) & 0x1f)
//Struct used to store dimension of the terminal
struct editorConfig
{
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

void disableRawMode();
void enableRawMode();
void die(const char* s);
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows();
int getWindowSize(int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);
void initEditor();
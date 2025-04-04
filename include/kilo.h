#pragma once
//Used to map Ctrl. combinations to specific terminal operations
//The bitwise operation sets the upper three bits to 0, which mirrors what the
//Ctrl. key does in the terminal
#define CTRL_KEY(k) ((k) & 0x1f)


void disableRawMode();
void enableRawMode();
void die(const char* s);
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
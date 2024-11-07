#ifndef UIEXTENSIONS_H
#define UIEXTENSIONS_H
#include "UI.h"
bool question(UI ui, const char *text) {
    ui.print(text);
    while (true) {
        char answer = _getch();
        if (tolower(answer) == 'y') return true;
        if (tolower(answer) == 'n') return false;
        Sleep(100);
    }
}

void expect(UI ui, const char *text, char expected) {
    ui.print(text);
    while (true) {
        char actual = _getch();
        if (actual == expected) return;
        Sleep(100);
    }
}
#endif

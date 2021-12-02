#include <windows.h>
#include <iostream>
#include <conio.h>
#include <fstream>
#include <stdexcept>
#include <locale.h>
#include <curses.h>
#include <string.h>
#include <sstream>

#pragma comment(lib, "winmm.lib")

struct UI {
    WINDOW *keymapper_window;
    int rows, cols;

    WINDOW *create_window(int height, int width, int starty, int startx, const char* label) {
        WINDOW* win;
        WINDOW* actual_window;

        win = newwin(height, width, starty, startx);
        box(win, 0 , 0);

        // add inner borders for label
        mvwaddch(win, 2, 0, ACS_LTEE);
        mvwhline(win, 2, 1, ACS_HLINE, width - 2);
        mvwaddch(win, 2, width - 1, ACS_RTEE);

        // print label in the middle
        mvwprintw(win, 1, (width - strlen(label)) / 2 - startx, "%s", label);

        wrefresh(win);

        actual_window = newwin(height - 4, width - 2, starty + 3, startx + 1);
        return actual_window;
    }

    UI() {
        initscr();
        cbreak();
        keypad(stdscr, TRUE);

        getmaxyx(stdscr, rows, cols);
    
        keymapper_window = create_window(rows, cols, 0, 0, "MIDI-device to keyboard mapper");
        scrollok(keymapper_window, TRUE);
    }

    ~UI() {
        endwin();
    }

    print(const char *message, ...) {
        va_list args;

        wscrl(keymapper_window, 1);
        wmove(keymapper_window, rows - 5, 0);

        va_start(args, message);
        vwprintw(keymapper_window, message, args);
        va_end(args);

        wrefresh(keymapper_window);
    }

    printchars(const char *message, ...) {
        va_list args;

        va_start(args, message);
        vwprintw(keymapper_window, message, args);
        va_end(args);

        wrefresh(keymapper_window); 
    }
};

bool isConfigured = false;
UI ui;
char cmkc; // current midi key code
char cmkp; // is current midi key is pressed or released
char *keymap = (char *) malloc(sizeof(char)*256); // idx is midi key code (0-127, typically), value is vk code (1-254)
int currentDevice; // current midi device
MMRESULT result;
HMIDIIN hMidiDevice;

void CALLBACK MICallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (wMsg == MIM_DATA) {
        cmkc = (dwParam1 & 0x0000ff00) >> 8;   // keycode
        cmkp = (dwParam1 & 0x00ff0000) >> 16;  // key released if 0, else pressed

        if (isConfigured) {
            if (cmkp) ui.print("Pressed  MIDI #%d", +cmkc);
            if (!cmkp) ui.print("Released MIDI #%d", +cmkc);
            if(keymap[+cmkc] != 0) {
                ui.printchars("\t mapped to KB #%c", keymap[+cmkc]);

                INPUT input;
                input.type = INPUT_KEYBOARD;
                input.ki.wScan = 0;
                input.ki.time = 0;
                input.ki.dwExtraInfo = 0;
                input.ki.wVk = keymap[+cmkc];
                input.ki.dwFlags = cmkp ? 0 : KEYEVENTF_KEYUP;

                SendInput(1, &input, sizeof(INPUT));
            } else {
                std::cout << std::endl;
            }
        }
    }
    return;
}

struct Keymapper {
    Keymapper () {
        currentDevice = 0;
        cmkc = 0;
        cmkp = 0;
        keymap = (char *)malloc(sizeof(char)*256);
        for (size_t i=0; i < 255; i++) {
            keymap[i] = 0;
        }
        hMidiDevice = NULL;
    }

    ~Keymapper () {
        midiInStop(hMidiDevice);
        midiInClose(hMidiDevice);
        hMidiDevice = NULL;
    }

    void selectActiveDevice() {
        currentDevice = 0;

        int numDevs = midiInGetNumDevs();
        if (numDevs == 0) {
            ui.print("No MIDI-devices detected");
            throw std::invalid_argument("No MIDI-devices detected");

            return;
        }
        
        if (numDevs > 1) {
            ui.print("Multiple MIDI-devices detected");
            MIDIINCAPS cur;
            for (size_t i=0; i<numDevs; i++) {
                midiInGetDevCaps(i, &cur, sizeof(MIDIINCAPS));
                ui.print("#%d: %s", i, cur.szPname);
            }

            int choice = -1;
            int input;
            int selectedNumber;
            std::stringstream inputStream;
            while (choice >= numDevs || choice < 0) {
                ui.print("Enter valid device number: ");
                choice = -1;
                inputStream.clear();
                inputStream.str("");
                while((input = _getch()) != VK_RETURN) {
                    selectedNumber = input - '0';
                    if (selectedNumber < 0 || selectedNumber > 9) continue;
                    inputStream << std::to_string(selectedNumber);
                    ui.printchars(std::to_string(selectedNumber).c_str());
                }
                inputStream >> choice;
            }

            currentDevice = choice;
        }

        MIDIINCAPS cur;
        midiInGetDevCaps(currentDevice, &cur, sizeof(MIDIINCAPS));
        ui.print("Current device: %s", cur.szPname);

        return;
    }

    void saveConfig() {
        std::ofstream out;
        out.open("keymap.cfg");
        if (out) out.write(keymap, 256);
        out.close();
        return;
    }

    void readConfig() {
        std::ifstream in;
        in.open("keymap.cfg");
        if (in) in.getline(keymap, 256);
        in.close();
        return;
    }

    void printConfig() {
        ui.print("Current config in keymap.cfg:");
        for(size_t i = 0; i < 255; ++i) {
            if (keymap[i] != 0) {
                ui.print("\t MIDI #%d : KB #%d", i, keymap[i]);
            }
        }
        return;
    }

    void reassignKeymap() {
        char midiKey;
        char vkCode;
        ui.print("To stop press Escape");
        ui.print("Press key to be emulated on the keyboard, then MIDI-key");
        while(true) {
            Sleep(100);
            vkCode = _getch();
            if (vkCode == VK_ESCAPE) break;
            else {
                const char* name = setlocale(LC_ALL, "");
                std::setlocale(LC_ALL, name);
                vkCode = std::toupper(vkCode);
                ui.print("KB: %d \t", vkCode); 
                cmkc = 0;
                while (cmkc == 0) {
                    Sleep(100);
                }
                midiKey = cmkc;
                ui.printchars("MIDI: %d", midiKey);
                keymap[+midiKey] = +vkCode;
            }
        }
        saveConfig();
        return;
    }

    void listen() {
        result = midiInOpen(&hMidiDevice, currentDevice, (DWORD)(void*)MICallback, 0, CALLBACK_FUNCTION);
        midiInStart(hMidiDevice);
    }
};


int main() {
    Keymapper keymapper = Keymapper();

    keymapper.selectActiveDevice();
    keymapper.readConfig();
    keymapper.printConfig();
    keymapper.listen();

    ui.print("Reassign mappings? Type \"Y\" or \"y\" to confirm");
    char reassign = _getch();
    if (reassign == 'Y' || reassign == 'y') {
        keymapper.reassignKeymap();
    }

    isConfigured = true;

    ui.print("Started mapping...");
    while(true) {
        int c = _getch();
        if (c == VK_ESCAPE) break;
    }

    keymapper.~Keymapper();
    free(keymap);
    ui.~UI();
    return 0;
}

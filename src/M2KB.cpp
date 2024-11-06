#include <windows.h>
#include <iostream>
#include <conio.h>

#include <stdexcept>
#include <locale.h>
#include <string.h>
#include <sstream>

#include <map>
#include "UI.h"
#include "Config.h"

#pragma comment(lib, "winmm.lib")

bool isConfigured = false;
UI ui = UI("MIDI-device to keyboard mapper");
Config config;

char cmkc; // current midi key code
char cmkp; // is current midi key is pressed or released
int currentDevice; // current midi device
MMRESULT result;
HMIDIIN hMidiDevice;

void CALLBACK MICallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (wMsg == MIM_DATA) {
        cmkc = (dwParam1 & 0x0000ff00) >> 8;   // keycode
        if (config.altcmkp == 0) cmkp = (dwParam1 & 0x00ff0000) >> 16;  // key released if 0, else pressed
        else if (config.altcmkp == 1) cmkp = ((dwParam1 & 0x00ff0000) >> 16) != 64;  // key released if 64, else pressed

        if (isConfigured) {
            if (cmkp) ui.print("Pressed  MIDI #%d", +cmkc);
            if (!cmkp) ui.print("Released MIDI #%d", +cmkc);
            if(config.keymap[+cmkc] != 0) {
                ui.printchars("\t mapped to KB #%c", config.keymap[+cmkc]);

                INPUT input;
                input.type = INPUT_KEYBOARD;
                input.ki.wScan = 0;
                input.ki.time = 0;
                input.ki.dwExtraInfo = 0;
                input.ki.wVk = config.keymap[+cmkc];
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

    void printConfig() {
        ui.print("Current keymap:");
        for(size_t i = 0; i < 255; ++i) {
            if (config.keymap[i] != 0) {
                ui.print("\t MIDI #%d : KB #%d", i, config.keymap[i]);
            }
        }
        ui.print("Current config:");
        for(const auto &fieldPair : config.getEntries()) {
            ui.print("\t%s:\t%s", fieldPair.first.c_str(), fieldPair.second.c_str());
        }
        return;
    }

    void changeSettings() {
        char vkCode;
        ui.print("Use alternative key release detection? Type \"Y\" or \"y\" to confirm");
        vkCode = _getch();
        config.altcmkp = (vkCode == 'Y' || vkCode == 'y');
        config.save();
        return;
    }

    void reassignKeymap() {
        char midiKey;
        char vkCode;
        ui.print("To stop press Escape");
        ui.print("Press key to be emulated on the keyboard, then MIDI-key");
        while (true) {
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
                config.keymap[+midiKey] = +vkCode;
            }
        }
        config.save();
        return;
    }

    void listen() {
        result = midiInOpen(&hMidiDevice, currentDevice, (DWORD)(void*)MICallback, 0, CALLBACK_FUNCTION);
        midiInStart(hMidiDevice);
    }
};

int main() {
    config = Config();
    config.load();
    Keymapper keymapper = Keymapper();
    keymapper.selectActiveDevice();
    keymapper.printConfig();
    keymapper.listen();

    ui.print("Change settings? Type \"Y\" or \"y\" to confirm");
    char changeSettings = _getch();
    if (changeSettings == 'Y' || changeSettings == 'y') {
        keymapper.changeSettings();
    }

    ui.print("Reassign mappings? Type \"Y\" or \"y\" to confirm");
    char reassign = _getch();
    if (reassign == 'Y' || reassign == 'y') {
        keymapper.reassignKeymap();
    }

    isConfigured = true;

    ui.print("Started mapping...");
    while (true) {
        int c = _getch();
        if (c == VK_ESCAPE) break;
    }

    keymapper.~Keymapper();
    config.~Config();
    ui.~UI();
    return 0;
}

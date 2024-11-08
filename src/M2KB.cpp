#include <windows.h>
#include <iostream>
#include <conio.h>
#include <stdexcept>
#include <locale.h>
#include <string.h>
#include <sstream>
#include <map>
#include "UI.h"
#include "UIExtensions.h"
#include "Config.h"

#pragma comment(lib, "winmm.lib")

class Keymapper {
    public:
        UI ui = UI("MIDI-device to keyboard mapper");
        Config config;

        int currentDevice = -1; // current midi device
        char cmkc = 0; // current midi key code
        char cmkp = 0; // is current midi key is pressed or released
        HMIDIIN hMidiDevice = NULL;
        bool mapping = false;

        ~Keymapper () {
            mapping = false;
            midiInStop(hMidiDevice);
            midiInClose(hMidiDevice);
            hMidiDevice = NULL;
            config.~Config();
            ui.~UI();
        }

        void selectActiveDevice() {
            currentDevice = -1;
            int numDevs = midiInGetNumDevs();
            if (numDevs == 0) {
                throw std::invalid_argument("No MIDI-devices detected");
            } else if (numDevs == 1) {
                currentDevice = 0;
            } else {
                ui.print("Multiple MIDI-devices detected");
                MIDIINCAPS cur;
                for (size_t i = 0; i < numDevs; i++) {
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
        }

        void printConfig() {
            ui.print("Current keymap:");
            for(size_t i = 0; i < 255; ++i) {
                if (config.keymap[i] != 0) {
                    ui.print("\t MIDI #%d : KB #%d", i, config.keymap[i]);
                }
            }
            ui.print("Current config:");
            for(const auto &fieldPair : config.repr()) {
                ui.print("\t%s:\t%s", fieldPair.first.c_str(), fieldPair.second.c_str());
            }
        }

        void changeSettings() {
            config.altcmkp = question(ui, "Use alternative key release detection? (y/n)");
            config.hwsc = question(ui, "Use hardware scan codes? (y/n)");
            config.save();
        }

        void reassignKeymap() {
            char midiKey;
            char vkCode;
            ui.print("To stop press Escape");
            ui.print("Press key to be emulated on the keyboard, then MIDI-key");
            while (true) {
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
                Sleep(100);
            }
            config.save();
            return;
        }

        void reportKey(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
            if (wMsg != MIM_DATA || !mapping) return;
            if (cmkp) ui.print("Pressed  MIDI #%d", +cmkc);
            if (!cmkp) ui.print("Released MIDI #%d", +cmkc);
            if(config.keymap[+cmkc] != 0) ui.printchars("\t mapped to KB #%c", config.keymap[+cmkc]);
        }

        void extractKey(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
            if (wMsg != MIM_DATA) return;
            cmkc = (dwParam1 & 0x0000ff00) >> 8; // keycode
            if (config.altcmkp == 0) cmkp = (dwParam1 & 0x00ff0000) >> 16; // key released if 0, else pressed
            else if (config.altcmkp == 1) cmkp = ((dwParam1 & 0x00ff0000) >> 16) != 64; // key released if 64, else pressed
        }

        void mapKey(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
            if (wMsg != MIM_DATA || !mapping) return;
            if (config.keymap[+cmkc] == 0) {
                std::cout << std::endl;
                return;
            }
            INPUT input;
            input.type = INPUT_KEYBOARD;
            input.ki.time = 0;
            input.ki.dwExtraInfo = 0;
            input.ki.wVk = config.keymap[+cmkc];
            if (config.hwsc) {
                input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
                input.ki.dwFlags = cmkp ? KEYEVENTF_SCANCODE : KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            } else {
                input.ki.wScan = 0;
                input.ki.dwFlags = cmkp ? 0 : KEYEVENTF_KEYUP;
            }
            SendInput(1, &input, sizeof(INPUT));
        }

        static void CALLBACK MICallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
            Keymapper* keymapper = reinterpret_cast <Keymapper*>(dwInstance);
            keymapper->extractKey(hMidiIn, wMsg, dwInstance, dwParam1, dwParam2);
            keymapper->mapKey(hMidiIn, wMsg, dwInstance, dwParam1, dwParam2);
            keymapper->reportKey(hMidiIn, wMsg, dwInstance, dwParam1, dwParam2);
        }

        void listen() {
            MMRESULT result = midiInOpen(&hMidiDevice, currentDevice, (DWORD_PTR)(void*)&Keymapper::MICallback, reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
            midiInStart(hMidiDevice);
        }
};

int main() {
    Keymapper keymapper = Keymapper();
    keymapper.config.use("config.ini", "keymap.cfg");
    keymapper.config.load();

    while (keymapper.currentDevice == -1) {
        try {
            keymapper.selectActiveDevice();
        } catch (std::invalid_argument const& e) {
            keymapper.ui.print("No MIDI-devices detected");
            Sleep(100);
        }
    }
    keymapper.printConfig();
    keymapper.listen();

    if (question(keymapper.ui, "Change settings? (y/n)")) keymapper.changeSettings();
    if (question(keymapper.ui, "Reassign mappings? (y/n)")) keymapper.reassignKeymap();

    keymapper.mapping = true;
    expect(keymapper.ui, "Started mapping... Press ESC to quit.", VK_ESCAPE);
    
    keymapper.~Keymapper();
    return 0;
}

**MIDI to keyboard mapper**

[![Build Status](https://travis-ci.com/k5md/M2KB.svg?branch=master)](https://travis-ci.com/k5md/M2KB)

## About
This program allows to map MIDI-keys to keyboard keys, i.e one's pressing a key on MIDI-keyboard triggers mapped keypress on a PC-keyboard. I've essentially written this tool for OSU!Mania, since it doesn't support MIDI input devices.

## Usage
[![M2KB763d538ac10b8bf0.gif](https://s3.gifyu.com/images/M2KB763d538ac10b8bf0.gif)](https://gifyu.com/image/kVFd)

Plug in your MIDI-keyboard or whatever MIDI-, run M2KB, if you don't have it configured previously, map midi keys to keyboard by pressing KB key, then MIDI one, press ESC, config will be saved in keymap.cfg in the program directory, so that you won't have to remap keys everytime you run the program, then M2KB enters mapping mode.

## Build
You can either build this app yourself from source or download a precompiled binary from the [releases page](https://github.com/k5md/M2KB/releases).

### Compiling
1. install docker
2. `make build`

## Platform Support
Only Windows is supported.

## Dependencies
This program uses [PDCurses](https://pdcurses.org/)

# HACK\*MATCH for the TI-84+ CE

Game by Zachtronics

Port by euphory

Support the official release! Check out *EXAPUNKS*, an incredible open-ended programming puzzle game and good introduction to Zachtronics's catalog; *Last Call BBS*, a collection best described as several outstanding games rolled into one; or check out the NES port this is most closely based on on itch.io.

## Installation

Using any linking program (TILP, TI Connect CE, etc), import HACKMTCH.8xg onto your calculator. This program also relies on the standard C library available [here](https://github.com/CE-Programming/libraries/releases). If launching the program displays the error "need libload", try reinstalling that library.

## How to Play

*HACK\*MATCH* is a match-4 game in the vein of Candy Crush, but (hopefully!) better. Move matching files into groups of 4 to remove them and score points. Files will continue to be added from the top; if they reach the bottom, it's game over!

Groups larger than 4 score bonus points. Two stars placed next to each other will destroy all matching files, but score no points. The higher your score gets, the faster the files will come!

## Controls

- Arrow keys: move EXA (cursor) left/right
- 2nd: grab/drop file
- alpha: swap top files
- clear: quit game (at any time)

## Technical Information

Format: auto-extracting 8xg

Program Type: ASM

Size and Variable Usage:
- RAM: 7705 B
    - HACKMTCH: 7685 B
    - HKMCHDAT: 20 B
- ARC: 54117 B
    - HKMCHGFX: 54117 B

## Source

https://github.com/ariajd27/hack-match-ce

# ----------------------------
# Makefile Options
# ----------------------------

NAME = HACKMTCH
DESCRIPTION = "A Tetris-like game of block-moving and -swapping designed by Zachtronics"
COMPRESSED = YES
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

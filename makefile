# ----------------------------
# Makefile Options
# ----------------------------

NAME = HCKMATCH
DESCRIPTION = "A Tetris-like game of block-moving and -swapping designed by Zachtronics"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

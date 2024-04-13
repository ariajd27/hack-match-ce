# ----------------------------
# Makefile Options
# ----------------------------

NAME = HACKMTCH
DESCRIPTION = "A fast-paced match-4 designed by Zachtronics"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

group::
	make gfx
	make
	convbin -j 8x -i bin/HACKMTCH.8xp -i src/gfx/HKMCHGFX.8xv -k 8xg-auto-extract -o bin/HACKMTCH.8xg -n HACKMTCH

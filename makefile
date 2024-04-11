# ----------------------------
# Makefile Options
# ----------------------------

NAME = HACKMTCH
DESCRIPTION = "A fast-paced match-4 designed by Zachtronics"
COMPRESSED = YES
ARCHIVED = NO

DEPS = src/gfx/gfx.h

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

src/gfx/gfx.h:
	make gfx

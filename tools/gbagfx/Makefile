CC ?= gcc

PKG_CONFIG ?= pkg-config
LIBPNG_CFLAGS := $(shell command -v $(PKG_CONFIG) >/dev/null 2>&1 && $(PKG_CONFIG) --cflags libpng 2>/dev/null)
LIBPNG_LDFLAGS := $(shell command -v $(PKG_CONFIG) >/dev/null 2>&1 && $(PKG_CONFIG) --libs-only-L libpng 2>/dev/null)

CFLAGS = -Wall -Wextra -Werror -Wno-sign-compare -std=c11 -O3 -flto -DPNG_SKIP_SETJMP_CHECK
CFLAGS += $(LIBPNG_CFLAGS)

LIBS = -lpng -lz
LDFLAGS += $(LIBPNG_LDFLAGS)

SRCS = main.c convert_png.c gfx.c jasc_pal.c lz.c rl.c util.c font.c huff.c

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean check-libpng

all: gbagfx$(EXE)
	@:

check-libpng:
	@printf '%s\n' '#include <png.h>' 'int main(void) { return 0; }' > .check_libpng.c
	@$(CC) $(CFLAGS) -c .check_libpng.c -o .check_libpng.o >/dev/null 2>&1 || { \
		status=$$?; \
		echo "ERROR: missing libpng headers for tools/gbagfx."; \
		echo "Checked header: png.h"; \
		echo "Checked compile command: $(CC) $(CFLAGS) -c .check_libpng.c -o .check_libpng.o"; \
		echo "Windows hint: MSYS2 -> pacman -S mingw-w64-x86_64-libpng"; \
		echo "Install libpng development headers so png.h is available to the host compiler, then rerun make."; \
		$(RM) .check_libpng.c .check_libpng.o .check_libpng$(EXE); \
		exit $$status; \
	}
	@$(CC) .check_libpng.o -o .check_libpng$(EXE) $(LDFLAGS) $(LIBS) >/dev/null 2>&1 || { \
		status=$$?; \
		echo "ERROR: missing libpng library for tools/gbagfx."; \
		echo "Checked link command: $(CC) .check_libpng.o -o .check_libpng$(EXE) $(LDFLAGS) $(LIBS)"; \
		echo "Windows hint: MSYS2 -> pacman -S mingw-w64-x86_64-libpng"; \
		echo "Install libpng development libraries so the host linker can resolve -lpng and -lz, then rerun make."; \
		$(RM) .check_libpng.c .check_libpng.o .check_libpng$(EXE); \
		exit $$status; \
	}
	@$(RM) .check_libpng.c .check_libpng.o .check_libpng$(EXE)

gbagfx-debug$(EXE): $(SRCS) convert_png.h gfx.h global.h jasc_pal.h lz.h rl.h util.h font.h
	$(CC) $(CFLAGS) -DDEBUG $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

gbagfx$(EXE): $(SRCS) convert_png.h gfx.h global.h jasc_pal.h lz.h rl.h util.h font.h
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) gbagfx gbagfx.exe

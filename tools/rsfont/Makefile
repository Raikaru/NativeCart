CC ?= gcc

PKG_CONFIG ?= pkg-config
LIBPNG_CFLAGS := $(shell command -v $(PKG_CONFIG) >/dev/null 2>&1 && $(PKG_CONFIG) --cflags libpng 2>/dev/null)
LIBPNG_LDFLAGS := $(shell command -v $(PKG_CONFIG) >/dev/null 2>&1 && $(PKG_CONFIG) --libs-only-L libpng 2>/dev/null)

CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -DPNG_SKIP_SETJMP_CHECK
CFLAGS += $(LIBPNG_CFLAGS)

LIBS = -lpng -lz
LDFLAGS += $(LIBPNG_LDFLAGS)

SRCS = main.c convert_png.c util.c font.c

.PHONY: all clean check-libpng

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

all: rsfont$(EXE)
	@:

check-libpng:
	@printf '%s\n' '#include <png.h>' 'int main(void) { return 0; }' > .check_libpng.c
	@$(CC) $(CFLAGS) -c .check_libpng.c -o .check_libpng.o >/dev/null 2>&1 || { \
		status=$$?; \
		echo "ERROR: missing libpng headers for tools/rsfont."; \
		echo "Checked header: png.h"; \
		echo "Checked compile command: $(CC) $(CFLAGS) -c .check_libpng.c -o .check_libpng.o"; \
		echo "Windows hint: MSYS2 -> pacman -S mingw-w64-x86_64-libpng"; \
		echo "Install libpng development headers so png.h is available to the host compiler, then rerun make."; \
		$(RM) .check_libpng.c .check_libpng.o .check_libpng$(EXE); \
		exit $$status; \
	}
	@$(CC) .check_libpng.o -o .check_libpng$(EXE) $(LDFLAGS) $(LIBS) >/dev/null 2>&1 || { \
		status=$$?; \
		echo "ERROR: missing libpng library for tools/rsfont."; \
		echo "Checked link command: $(CC) .check_libpng.o -o .check_libpng$(EXE) $(LDFLAGS) $(LIBS)"; \
		echo "Windows hint: MSYS2 -> pacman -S mingw-w64-x86_64-libpng"; \
		echo "Install libpng development libraries so the host linker can resolve -lpng and -lz, then rerun make."; \
		$(RM) .check_libpng.c .check_libpng.o .check_libpng$(EXE); \
		exit $$status; \
	}
	@$(RM) .check_libpng.c .check_libpng.o .check_libpng$(EXE)

rsfont$(EXE): $(SRCS) convert_png.h gfx.h global.h util.h font.h
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) rsfont rsfont.exe

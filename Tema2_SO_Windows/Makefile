#.PHONY: clean

CFLAGS = /W3 /nologo /DDLL_EXPORTS /c

all: build

build: so_stdio.obj
	link /nologo /dll /out:so_stdio.dll /implib:so_stdio.lib so_stdio.obj

so_stdio.obj: so_stdio.c
	cl $(CFLAGS) so_stdio.c

clean:
	del *.lib
	del *.obj
	del *.dll
	del *.exp
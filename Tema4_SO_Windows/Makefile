CFLAGS = /nologo /W3 /EHsc
CC = cl
LINK = link

all: build

build: libscheduler.dll

libscheduler.dll: so_scheduler.obj
	$(LINK) /nologo /dll /out:$@ /implib:libscheduler.lib $**

so_scheduler.obj: so_scheduler.c
	$(CC) $(CFLAGS) /Fo$@ /c $**

.PHONY: clean
clean:
	del /Q so_scheduler.obj 2>NUL
	del /Q libscheduler.dll libscheduler.lib libscheduler.exp 2>NUL

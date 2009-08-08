TARGET = All
OBJS = main.o prx/supportlib.o fileman/filebrowser.o gfx/graphics.o gfx/framebuffer.o audio/mp3playerME.o audio/id3.o intraFont/libccc.o intraFont/intraFont.o lrc/lrc.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpng -ljpeg -lpsppower -lz -lm -lpspdebug -lpspgu -lpspkubridge -lpspaudiolib -lpspaudio -lpspaudiocodec -lpspsystemctrl_user -lpsprtc

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = 
PSP_EBOOT_ICON = ICON.png 
PSP_EBOOT_PIC1 = PIC1.png 

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

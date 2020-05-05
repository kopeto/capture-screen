SRC = capture.c

TARGET = capture

SDL_LIBS = $(shell sdl2-config --libs)
INCLUDES = $(shell sdl2-config --cflags)

AV_INCLUDES = $(shell pkg-config --cflags libavformat)
AV_LIBS  = $(shell pkg-config --libs libavformat libavutil libavcodec libavdevice libswscale libavfilter)

all: $(TARGET)

$(TARGET): $(SRC)
	gcc $^ $(INCLUDES) $(SDL_LIBS) $(AV_INCLUDES) $(AV_LIBS) -o $@

trans: transcodeng.c
	gcc $^ $(INCLUDES) $(SDL_LIBS) $(AV_INCLUDES) $(AV_LIBS) -o $@

screen-cast: screen_cast.c
	gcc $^ $(INCLUDES) $(SDL_LIBS) $(AV_INCLUDES) $(AV_LIBS) -o $@


clean:
	rm -f $(TARGET) trans
	rm -f *.o
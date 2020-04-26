SRC = capture.c

TARGET = capture

SDL_LIBS = $(shell sdl2-config --libs)
INCLUDES = $(shell sdl2-config --cflags)

AV_INCLUDES = $(shell pkg-config --cflags libavformat)
AV_LIBS  = $(shell pkg-config --libs libavformat libavutil libavcodec libavdevice libswscale)

all: $(TARGET)

$(TARGET): $(SRC)
	gcc $^ $(INCLUDES) $(SDL_LIBS) $(AV_INCLUDES) $(AV_LIBS) -o $@


clean:
	rm -f $(TARGET)
	rm -f *.o
SRC = capture.c

TARGET = capture

SDL_LIBS = $(shell sdl-config --libs)
#AV_LIBS  = $(shell pkg-config --libs libavformat libavutil libavcodec libavdevice libswscale) 
#AV_LIBS = -lavformat -lavutil -lavdevice -lavcodec -lm -lswscale
INCLUDES = $(shell sdl-config --cflags)


#AV_INCLUDES = -I/home/ander/ffmpeg_build/include
AV_INCLUDES = $(shell pkg-config --cflgas libavformat)
#AV_LIBS = -L/home/ander/ffmpeg_build/lib  -lavdevice  -lm  -lswresample  -lavformat -lswscale -lavutil -lavcodec 
AV_LIBS  = $(shell pkg-config --libs libavformat libavutil libavcodec libavdevice libswscale)

all: $(TARGET)

$(TARGET): $(SRC)
	gcc $^ $(INCLUDES) $(SDL_LIBS) $(AV_INCLUDES) $(AV_LIBS) -o $@


encode: encode.c
	gcc $^ $(AV_INCLUDES) $(AV_LIBS) -o $@

clean:
	rm -f $(TARGET)
	rm -f encode
	rm -f *.o
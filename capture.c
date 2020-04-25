
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL/SDL.h>

#include <SDL/SDL.h>

//Output YUV420P 
#define OUTPUT_YUV420P 0
//'1' Use Dshow 
//'0' Use GDIgrab
#define USE_DSHOW 0

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit=0;

int sfp_refresh_thread(void *opaque)
{
	thread_exit=0;
	while (!thread_exit) {
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit=0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}


int main(int argc, char* argv[])
{

	AVFormatContext	*pFormatCtx = NULL;
	int				i, videoindex;

    AVStream        *pStream = NULL;

    //AVCodecParameters *pCodecParams = nullptr;
	AVCodecContext	*pCodecCtx = NULL;
	AVCodecContext	*pCodecCtxOut = NULL;
	AVCodec			*pCodec = NULL;
	
	AVCodec* TEST;

	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	
	//Open File
	//char filepath[]="src01_480x272_22.h265";
	//avformat_open_input(&pFormatCtx,filepath,NULL,NULL)

	//Register Device
	avdevice_register_all();
	//avcodec_register_all();


	//Linux
	AVDictionary* options = NULL;
	//Set some options
	//grabbing frame rate
	//av_dict_set(&options,"framerate","5",0);
	//Make the grabbed area follow the mouse
	//av_dict_set(&options,"follow_mouse","centered",0);
	//Video frame size. The default is to capture the full screen
	//av_dict_set(&options,"video_size","640x480",0);
	AVInputFormat *ifmt=av_find_input_format("x11grab");

	const char* codec_name = "libx264";

    //codec = avcodec_find_encoder (AV_CODEC_ID_H264);
    pCodec = avcodec_find_encoder_by_name (codec_name);


    pCodec = avcodec_find_encoder_by_name( "libx264" );
    if(!pCodec)
    {
        printf("Codec not found.\n");
        return -1;
    }

    printf("Codec fucking name: %s\n", pCodec->long_name);
    
	//Grab at position 10,20
	if(avformat_open_input(&pFormatCtx,":1.0",ifmt,&options)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
            pStream = pFormatCtx->streams[i];
            printf("Video index: %d\n",videoindex);
			break;
		}

	if(!pStream)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}



	pCodecCtxOut = avcodec_alloc_context3(pCodec);

    /* put sample parameters */
    pCodecCtxOut->bit_rate = 400000;
    /* resolution must be a multiple of two */
    pCodecCtxOut->width = 1920;
    pCodecCtxOut->height = 1080;
    /* frames per second */
    pCodecCtxOut->time_base = (AVRational){1, 25};
    pCodecCtxOut->framerate = (AVRational){25, 1};

    pCodecCtxOut->gop_size = 10;
    pCodecCtxOut->max_b_frames = 1;
    pCodecCtxOut->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(pCodecCtxOut->priv_data, "preset", "fast", 0);

	// printf("pCodecCtxOut->width : %d\n",pCodecCtxOut->width);
	// printf("pCodecCtxOut->height : %d\n",pCodecCtxOut->height);

	if(avcodec_open2(pCodecCtxOut, pCodec, NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	printf("We arrived here.\n");

	AVFrame	*pFrame,*pFrameYUV;

	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();

	unsigned char *out_buffer=(unsigned char *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtxOut->width, pCodecCtxOut->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtxOut->width, pCodecCtxOut->height);
	
    
    //SDL----------------------------

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	}else{
        printf("SDL initiated\n");
    }
	int screen_w=640,screen_h=360;
	const SDL_VideoInfo *vi = SDL_GetVideoInfo();
	//Half of the Desktop's width and height.
	screen_w = vi->current_w/2;
	screen_h = vi->current_h/2;
	SDL_Surface *screen; 
	screen = SDL_SetVideoMode(screen_w, screen_h, 0,0);

	if(!screen) {  
		printf("SDL: could not set video mode - exiting:%s\n",SDL_GetError());  
		return -1;
	}else{
        printf("Screen set.\n");
    }


	SDL_Overlay *bmp = NULL; 
	bmp = SDL_CreateYUVOverlay(screen_w, screen_h, AV_PIX_FMT_BGR0 , screen); 
	if(!bmp){
        printf("Couldn't create bmp overlay\n");
        printf("ERROR: %s\n",SDL_GetError());
        return -1;
    }    else{
        printf("BMP overlay created");
    }
    SDL_Rect rect;
	rect.x = 0;    
	rect.y = 0;    
	rect.w = screen_w;    
	rect.h = screen_h;  
	//SDL End------------------------
	int ret, got_picture;

	AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));

#if OUTPUT_YUV420P 
    FILE *fp_yuv=fopen("output.yuv","wb+");  
#endif  

	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	//------------------------------
	SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread,NULL);
	//
	SDL_WM_SetCaption("Simplest FFmpeg Grab Desktop",NULL);
	//Event Loop
	SDL_Event event;

	for (;;) {
		//Wait
		SDL_WaitEvent(&event);
		if(event.type==SFM_REFRESH_EVENT){
			//------------------------------
			if(av_read_frame(pFormatCtx, packet)>=0){
				if(packet->stream_index==videoindex){
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
					if(ret < 0){
						printf("Decode Error.\n");
						return -1;
					}
					if(got_picture){
						SDL_LockYUVOverlay(bmp);
						pFrameYUV->data[0]=bmp->pixels[0];
						pFrameYUV->data[1]=bmp->pixels[2];
						pFrameYUV->data[2]=bmp->pixels[1];     
						pFrameYUV->linesize[0]=bmp->pitches[0];
						pFrameYUV->linesize[1]=bmp->pitches[2];   
						pFrameYUV->linesize[2]=bmp->pitches[1];
						sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

#if OUTPUT_YUV420P  
						int y_size=pCodecCtx->width*pCodecCtx->height;    
						fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y   
						fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U  
						fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V  
#endif  
						SDL_UnlockYUVOverlay(bmp); 
						
						SDL_DisplayYUVOverlay(bmp, &rect); 

					}
				}
				av_packet_free(&packet);
			}else{
				//Exit Thread
				thread_exit=1;
			}
		}else if(event.type==SDL_QUIT){
			thread_exit=1;
		}else if(event.type==SFM_BREAK_EVENT){
			break;
		}

	}


	sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P 
    fclose(fp_yuv);
#endif 

	SDL_Quit();

	//av_free(out_buffer);
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
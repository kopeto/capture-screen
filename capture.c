
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL.h>

int main(int argc, char* argv[])
{

	AVFormatContext	*pFormatCtx = NULL;
	int				i;

    AVStream        *pStream = NULL;

    //AVCodecParameters *pCodecParams = nullptr;
	AVCodecContext	*pCodecCtx = NULL;
	AVCodecContext	*pCodecCtxOut = NULL;

	AVCodec			*pCodecIn = NULL;
	AVCodec			*pCodecOut = NULL;

	SDL_Window		*window = NULL;
	SDL_Renderer	*renderer = NULL;
	SDL_Texture		*texture = NULL;

	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	
	//Register Device
	avdevice_register_all();

	//Linux
	AVDictionary* options = NULL;
	//Video frame size. The default is to capture the full screen
	av_dict_set(&options,"video_size","2520x1440",0);

	AVInputFormat *ifmt=av_find_input_format("x11grab");


	//Grab at position 10,20
	if(avformat_open_input(&pFormatCtx,":0.0",ifmt,&options)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	for(i=0; i<pFormatCtx->nb_streams; i++) 
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
		{
            pStream = pFormatCtx->streams[i];
            printf("Video index: %d\n",pStream->index);
			break;
		}

	if(!pStream)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	printf("Stream time base %d / %d\n", pStream->time_base.den, pStream->time_base.num );
	printf("Stream avg frame rate %.2ffps\n", (float) pStream->avg_frame_rate.num / (float) pStream->avg_frame_rate.den );
	printf("Stream display aspect ratio %d:%d\n", pStream->display_aspect_ratio.den, pStream->display_aspect_ratio.num );

	if(pStream->codecpar)
	{
		printf("Frames size %d x %d\n",pStream->codecpar->width, pStream->codecpar->height);
		printf("Sample aspect ratio %d:%d\n",pStream->codecpar->sample_aspect_ratio.den, pStream->codecpar->sample_aspect_ratio.num);
		printf("Codec id %d\n",pStream->codecpar->codec_id);
	}
	else
	{
		printf("There is no codec params\n");
	}



	//av_dump_format(pFormatCtx,pStream->index, pFormatCtx->filename, 0);

	// --------------------------------------
	// SETUP DECODER
	// --------------------------------------
	pCodecIn = avcodec_find_decoder(pStream->codecpar->codec_id);
	if(!pCodecIn)
	{
		printf("Couldn't find raw data decoder\n");
		return -1;
	}
	else
	{
		printf("Decoder is %s\n",pCodecIn->long_name);
	}

	pCodecCtx = avcodec_alloc_context3(pCodecIn);

	pCodecCtx->pix_fmt = pStream->codecpar->format;

	pCodecCtx->width = pStream->codecpar->width;
    pCodecCtx->height = pStream->codecpar->height;

	if(avcodec_open2(pCodecCtx, pCodecIn, NULL)<0)
	{
		printf("Could not open decoder.\n");
		return -1;
	}
	
	// --------------------------------------
	// SETUP ENCODER
	// --------------------------------------

	//codec = avcodec_find_encoder (AV_CODEC_ID_H264);
    pCodecOut = avcodec_find_encoder_by_name ("libx264");
    if(!pCodecOut)
    {
        printf("Codec not found.\n");
        return -1;
    }
    printf("Encoder name: %s\n", pCodecOut->long_name);

	pCodecCtxOut = avcodec_alloc_context3(pCodecOut);

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

	if(avcodec_open2(pCodecCtxOut, pCodecOut, NULL)<0)
	{
		printf("Could not open encoder.\n");
		return -1;
	}




	
    
    // SDL

	if(SDL_Init(SDL_INIT_EVERYTHING)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	}else{
        printf("SDL initiated\n");
    }

	window = SDL_CreateWindow("Window",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, pCodecCtxOut->width, pCodecCtxOut->height, 0);
	renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecCtxOut->width, pCodecCtxOut->height);

	//SDL End

	int ret;

	AVPacket *packet=av_packet_alloc();
	AVFrame	*pFrame,*pFrameYUV;

	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();

	unsigned char *out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtxOut->width, pCodecCtxOut->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtxOut->width, pCodecCtxOut->height,1);

	// FILE *fp_yuv=fopen("output.yuv","wb+");  

	struct SwsContext *img_convert_ctx = NULL;
	
	img_convert_ctx = sws_getContext(
							pCodecCtx->width, 
							pCodecCtx->height, 
							pCodecCtx->pix_fmt, 
							pCodecCtxOut->width, 
							pCodecCtxOut->height, 
							pCodecCtxOut->pix_fmt, 
							SWS_BICUBIC, 
							NULL, NULL, NULL); 
	
	//Event Loop
	SDL_Event event;
	int done = 0;
	int packet_number=0;
	
	while(!done) {
		//Wait
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
				{
					done = 1;
					break;
				}
				case SDL_KEYDOWN:
				{
					switch(event.key.keysym.sym)
					{
						case SDLK_q:
						case SDLK_ESCAPE:
						{
							done = 1;
							break;
						}
					}
				}
				default:
					break;
			}
		} // POLL EVENTS

		//------------------------------
		if(av_read_frame(pFormatCtx, packet)>=0){
			if(packet->stream_index==pStream->index){
				// printf("packet %d received\n", packet_number++);

				avcodec_send_packet(pCodecCtx, packet);
				avcodec_receive_frame(pCodecCtx, pFrame);

				// Frame info: 
				// linesize[0] = 10080 
				// 2520 x 4
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
				// printf("\tScaled frame: %dx%d\n",pFrameYUV->width, pFrameYUV->height);
				// for(int i=0; i<AV_NUM_DATA_POINTERS; ++i)
				// {
				// 	printf("\tScaled frame Data linesize[%d] %d\n",i,pFrameYUV->linesize[i]);
				// }

				if(SDL_UpdateYUVTexture(	texture,
										NULL,
										pFrameYUV->data[0],
										pFrameYUV->linesize[0],
										pFrameYUV->data[1],
										pFrameYUV->linesize[1],
										pFrameYUV->data[2],
										pFrameYUV->linesize[2]
				) < 0)
				{
					printf("Can't create YUV texture\n");
					printf("ERROR: %s\n",SDL_GetError());
				}
				SDL_RenderCopy(renderer, texture, NULL, NULL);
  				SDL_RenderPresent(renderer);

			}

		}else{
			done = 1;
		}	
	} // WHILE

	printf("Free resources...\n");

	av_packet_free(&packet);
	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
	
	if(img_convert_ctx)
		sws_freeContext(img_convert_ctx);
	if(renderer)
		SDL_DestroyRenderer(renderer);
	if(window)
		SDL_DestroyWindow(window);
	if(texture)
		SDL_DestroyTexture(texture);
	SDL_Quit();

	//av_free(out_buffer);
	if(pFrameYUV)
		av_free(pFrameYUV);
	if(pCodecCtx)
		avcodec_close(pCodecCtx);
	if(pFormatCtx)
		avformat_close_input(&pFormatCtx);

	return 0;
}

#include <stdio.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>

	static AVFormatContext	*pFormatCtx = NULL;
	static AVFormatContext	*pFormatCtxOut = NULL;

	static AVInputFormat 	*ifmt = NULL;

    static AVStream        	*pStream = NULL;
	static AVStream			*pStreamOut = NULL;

	static AVCodecContext	*pCodecCtx = NULL;
	static AVCodecContext	*pCodecCtxOut = NULL;

	static AVCodec			*pCodecIn = NULL;
	static AVCodec			*pCodecOut = NULL;

	static AVPacket 		*packet = NULL;
	static AVFrame			*pFrame = NULL;
	static AVFrame 			*pFrameYUV = NULL;

	static AVDictionary		*dec_options = NULL;
	static AVDictionary		*enc_options = NULL;

	static FILE 			*fp_yuv = NULL; 
	static const char 		*out_filename = "out.mp4";
	static const char		*screen_dimensions = "2520x1440";
	static const char		*target_dimensions = "1280x720";
	static const int		target_width = 1280;
	static const int		target_height = 720;

	static struct SwsContext *img_convert_ctx = NULL;

	static int done = 0;
	static int pts = 0;

	static uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	static unsigned char *out_buffer = NULL;




static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    int ret;

	/* prepare packet for muxing */
    // pkt->stream_index = pStream->index;
    // av_packet_rescale_ts(pkt,
    //                      pStreamOut->time_base,
    //                      pStreamOut->time_base);

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3"PRId64"\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
		    /* prepare packet for muxing */
		// pkt->stream_index = pStream->index;
		// av_packet_rescale_ts(pkt,
		// 					pStream->time_base,
		// 					pStreamOut->time_base);

		/* mux encoded frame */
		ret = av_write_frame(pFormatCtxOut, pkt);
			
        av_packet_unref(pkt);

		return;
    }
}

static int open_output_file(const char *filename)
{
    int ret;
    unsigned int i;

	// --------------------------------------
	// SETUP ENCODER
	// --------------------------------------


    avformat_alloc_output_context2(&pFormatCtxOut, NULL, NULL, filename);
    if (!pFormatCtxOut) {
        printf("Could not create output context\n");
        return -1;
    }

	pStreamOut = avformat_new_stream(pFormatCtxOut, NULL);

	if (!pStreamOut) {
		printf("Failed allocating output stream\n");
		return -1;
	}

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
    pCodecCtxOut->bit_rate = 600000000;
    /* resolution must be a multiple of two */
    pCodecCtxOut->width = target_width;
    pCodecCtxOut->height = target_height;
    /* frames per second */


    pCodecCtxOut->time_base = pStream->time_base;
    pCodecCtxOut->framerate = pStream->avg_frame_rate;
    // pCodecCtxOut->time_base = (AVRational){1, 25};
    // pCodecCtxOut->framerate = (AVRational){25, 1};

    pCodecCtxOut->gop_size = 10;
    pCodecCtxOut->max_b_frames = 1;
    pCodecCtxOut->pix_fmt = AV_PIX_FMT_YUV420P;

    // av_opt_set(pCodecCtxOut->priv_data, "preset", "slow", 0);
	// av_dict_set(&enc_options,"no-correct-pts",NULL,0);
	// av_dict_set(&enc_options,"fps","25.00",0);


	if (pFormatCtxOut->oformat->flags & AVFMT_GLOBALHEADER)
		pCodecCtxOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if(avcodec_open2(pCodecCtxOut, pCodecOut, &enc_options)<0)
	{
		printf("Could not open encoder.\n");
		return -1;
	}

	ret = avcodec_parameters_from_context(pStreamOut->codecpar, pCodecCtxOut);
	if (ret < 0) {
		printf( "Failed to copy encoder parameters to output stream\n");
		return ret;
	}

	pStreamOut->time_base = pCodecCtxOut->time_base;

    av_dump_format(pFormatCtxOut, 0, filename, 1);

    if (!(pFormatCtxOut->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&pFormatCtxOut->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("Could not open output file '%s'", filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(pFormatCtxOut, NULL);
	ret = av_write_trailer(pFormatCtxOut);

    if (ret < 0) {
        printf("Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}

int main(int argc, char* argv[])
{
	//avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	
	//Register Device
	avdevice_register_all();

	
	ifmt=av_find_input_format("x11grab");

	//Video frame size. The default is to capture the full screen
	av_dict_set(&dec_options,"video_size",screen_dimensions,0);
	if(avformat_open_input(&pFormatCtx,":0.0",ifmt,&dec_options)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	for(int i=0; i<pFormatCtx->nb_streams; i++) 
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
		printf("Frames size %d x %d\n", pStream->codecpar->width, pStream->codecpar->height);
		printf("Sample aspect ratio %d:%d\n", pStream->codecpar->sample_aspect_ratio.den, pStream->codecpar->sample_aspect_ratio.num);
		printf("Codec id %d\n", pStream->codecpar->codec_id);
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
		printf("Decoder is %s\n", pCodecIn->long_name);
	}

	pCodecCtx = avcodec_alloc_context3(pCodecIn);
	avcodec_parameters_to_context(pCodecCtx, pStream->codecpar);
	pCodecCtx->framerate = av_guess_frame_rate(pFormatCtx, pStream, NULL);

	printf("Sample aspect rate: %d:%d\n",pCodecCtx->sample_aspect_ratio.num,pCodecCtx->sample_aspect_ratio.den);
	printf("Guest frame rate: %d:%d\n",pCodecCtx->framerate.num,pCodecCtx->framerate.den);

	if(avcodec_open2(pCodecCtx, pCodecIn, NULL)<0)
	{
		printf("Could not open decoder.\n");
		return -1;
	}
	// --------------------------------------------------------------


	// SETUP ENCODER
	// --------------------------------------
	
	open_output_file(out_filename);	

	// --------------------------------------


	packet=av_packet_alloc();
	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();

	out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtxOut->width, pCodecCtxOut->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtxOut->width, pCodecCtxOut->height,1);

	pFrameYUV->format = AV_PIX_FMT_YUV420P;
	pFrameYUV->width = pCodecCtxOut->width;
	pFrameYUV->height = pCodecCtxOut->height;

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
	int number_packets = 0;
	while(!done && number_packets <200) {
		//Wait
		if(av_read_frame(pFormatCtx, packet)>=0){

			if(packet->stream_index==pStream->index){

				number_packets++;

				if(avcodec_send_packet(pCodecCtx, packet) != 0)
				{
					printf("Some error ocurred in avcodec_send_packet() function\n");
				}
				avcodec_receive_frame(pCodecCtx, pFrame);

				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

				pFrameYUV->pts = pts++;
				/* encode the image */
				encode(pCodecCtxOut, pFrameYUV, packet, fp_yuv);

			}

		}else{
			done = 1;
		}	

	} // WHILE

    /* flush the encoder */
	encode(pCodecCtxOut, NULL, packet, fp_yuv);

	av_write_trailer(pFormatCtxOut);
	
	printf("Free resources...\n");

	av_packet_free(&packet);
	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);

	if(img_convert_ctx)
		sws_freeContext(img_convert_ctx);

	if(fp_yuv)
		fclose( fp_yuv );

	if(out_buffer)
		av_free(out_buffer);
	if(pFrameYUV)
		av_free(pFrameYUV);
	if(pCodecCtx)
		avcodec_close(pCodecCtx);
	if(pFormatCtx)
		avformat_close_input(&pFormatCtx);

	return 0;
}
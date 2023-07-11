#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libavutil/imgutils.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

FILE *fDump = NULL;
bool saveJpg = true;
static void SaveAvFrame(AVFrame *avFrame)
{
    uint32_t pitchY = avFrame->linesize[0];
    uint32_t pitchU = avFrame->linesize[1];
    uint32_t pitchV = avFrame->linesize[2];

    uint8_t *avY = avFrame->data[0];
    uint8_t *avU = avFrame->data[1];
    uint8_t *avV = avFrame->data[2];

    for (uint32_t i = 0; i < avFrame->height; i++) {
        fwrite(avY, avFrame->width, 1, fDump);
        avY += pitchY;
    }

    for (uint32_t i = 0; i < avFrame->height/2; i++) {
        fwrite(avU, avFrame->width/2, 1, fDump);
        avU += pitchU;
    }

    for (uint32_t i = 0; i < avFrame->height/2; i++) {
        fwrite(avV, avFrame->width/2, 1, fDump);
        avV += pitchV;
    }
    // save to jpg
    if (saveJpg)
    {
        FILE *outputFile = fopen("test.jpg", "ab");
        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);

        // Set codec parameters
        codecContext->bit_rate = 400000;
        codecContext->width = avFrame->width;
        codecContext->height = avFrame->height;
        codecContext->time_base = (AVRational){1, 60};
        codecContext->framerate = (AVRational){60, 1};
        codecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;

        avcodec_open2(codecContext, codec, NULL);
        AVPacket *packet = av_packet_alloc();
        int ret = avcodec_send_frame(codecContext, avFrame);
        if (ret < 0)
        {
            printf("avcodec_send_frame error:%d\n", ret);
            return;
        }
        while (ret >= 0)
        {
            ret = avcodec_receive_packet(codecContext, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                // No more packets to receive
                break;
            }
            else if (ret < 0)
            {
                // Error during packet reception
                break;
            }

            // Store the packet to a file
            fwrite(packet->data, 1, packet->size, outputFile);
            av_packet_unref(packet);
        }

        saveJpg = false;
        fclose(outputFile);
    }
}

FILE *f264Dump = NULL;
static void SaveH264(uint8_t *data, int size)
{
    fwrite(data, size, 1, f264Dump);
}

int main() {
    // Path to input video file
    const char* inputFilePath = "input.mov";
    
    // Path to output video file
    const char* outputFilePath = "output.mp4";
    
    // Path to output image file
    const char* imageFilePath = "output.jpg";
    
    // Duration of the desired segment (in seconds)
    float segmentDuration = 10.f;
    
    // Video resolution after scaling
    int scaledWidth = 960;   // 540p
    int scaledHeight = 540;
   av_log_set_level(AV_LOG_TRACE);
 
    // Initialize FFmpeg network components
    avformat_network_init();
    remove("test.yuv");
    remove("output.mp4");
    remove("test.264");
    fDump = fopen("test.yuv", "ab");
    f264Dump = fopen("test.264", "ab");
    // Open input video file
    AVFormatContext* formatContext = NULL;
    if (avformat_open_input(&formatContext, inputFilePath, NULL, NULL) != 0) {
        printf("Failed to open input video file.\n");
        return -1;
    }
    
    // Find stream information
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        printf("Failed to retrieve stream information.\n");
        avformat_close_input(&formatContext);
        return -1;
    }
    
    // Find video stream
    int videoStreamIndex = -1;
  int stream_index = 0;
    AVStream* videoStream = NULL;
    int streams_list[100] = {0};

    if (!streams_list)
    {
        return AVERROR(ENOMEM);
    }
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            videoStream = formatContext->streams[i];
            break;
        }
       
    }
    
    if (videoStream == NULL) {
        printf("Failed to find video stream.\n");
        avformat_close_input(&formatContext);
        return -1;
    }
    
    
    // Initialize video decoder
    AVCodec* videoCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (videoCodec == NULL) {
        printf("Failed to find video codec.\n");
        avformat_close_input(&formatContext);
        return -1;
    }
    AVCodecContext* videoCodecContext = avcodec_alloc_context3(videoCodec);
    if (avcodec_parameters_to_context(videoCodecContext, videoStream->codecpar) < 0) {
        printf("Failed to initialize video codec context.\n");
        avformat_close_input(&formatContext);
        return -1;
    }

    // Set the time base for the input video codec context
    videoCodecContext->time_base = videoStream->time_base;
    if (avcodec_open2(videoCodecContext, videoCodec, NULL) < 0) {
        printf("Failed to open video codec.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&videoCodecContext);
        return -1;
    }
    
    // Find video frame rate
    AVRational videoFrameRate = videoStream->avg_frame_rate;
    int videoFrameRateNum = videoFrameRate.num;
    int videoFrameRateDen = videoFrameRate.den;
    
    // Initialize video scaler
    AVFrame* frame = av_frame_alloc();
    AVFrame* scaledFrame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, scaledWidth, scaledHeight, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(scaledFrame->data, scaledFrame->linesize, buffer, AV_PIX_FMT_YUV420P, scaledWidth, scaledHeight, 1);
    struct SwsContext* scalerContext = sws_getContext(
        videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt,
        scaledWidth, scaledHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    // Initialize output format
    AVOutputFormat* outputFormat = av_guess_format(NULL, outputFilePath, NULL);
    if (outputFormat == NULL) {
        printf("Failed to guess output format.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&videoCodecContext);
        av_frame_free(&frame);
        av_frame_free(&scaledFrame);
        av_free(buffer);
        sws_freeContext(scalerContext);
        return -1;
    }
    
    // Open output video file
    AVFormatContext* outputFormatContext = NULL;
    if (avformat_alloc_output_context2(&outputFormatContext, outputFormat, NULL, outputFilePath) < 0) {
        printf("Failed to allocate output format context.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&videoCodecContext);
        av_frame_free(&frame);
        av_frame_free(&scaledFrame);
        av_free(buffer);
        sws_freeContext(scalerContext);
        return -1;
    }
    
    // Find output video codec
    AVCodec* outputVideoCodec = avcodec_find_encoder(outputFormat->video_codec);
    if (outputVideoCodec == NULL) {
        printf("Failed to find output video codec.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&videoCodecContext);
        av_frame_free(&frame);
        av_frame_free(&scaledFrame);
        av_free(buffer);
        sws_freeContext(scalerContext);
        avformat_free_context(outputFormatContext);
        return -1;
    }
AVStream* outputVideoStream = NULL;
    for (int i = 0; i < formatContext->nb_streams; i++)
    {
        AVStream *out_stream;
        AVStream *in_stream = formatContext->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
        {
            streams_list[i] = -1;
            continue;
        }
        streams_list[i] = stream_index++;
        out_stream = avformat_new_stream(outputFormatContext, NULL);
        if (!out_stream)
        {
            fprintf(stderr, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }
        if (avcodec_parameters_copy(out_stream->codecpar, in_codecpar) < 0)
        {
            fprintf(stderr, "Failed to copy codec parameters\n");
            return -1;
        }
        if (i == videoStreamIndex)
        {
            outputVideoStream = out_stream;
        }
    }
    // Add output video stream
    AVCodecContext *outputVideoCodecContext = avcodec_alloc_context3(outputVideoCodec);
    outputVideoCodecContext->time_base = av_inv_q(videoFrameRate);
    outputVideoCodecContext->width = scaledWidth;
    outputVideoCodecContext->height = scaledHeight;
    outputVideoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    outputVideoCodecContext->sample_aspect_ratio = videoCodecContext->sample_aspect_ratio;
    printf("input pixel format:%d\n", videoCodecContext->pix_fmt);
    if (avcodec_open2(outputVideoCodecContext, outputVideoCodec, NULL) < 0) {
        printf("Failed to open output video codec.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&videoCodecContext);
        avcodec_free_context(&outputVideoCodecContext);
        av_frame_free(&frame);
        av_frame_free(&scaledFrame);
        av_free(buffer);
        sws_freeContext(scalerContext);
        avformat_free_context(outputFormatContext);
        return -1;
    }
    // fill paramter
    if (avcodec_parameters_from_context(outputVideoStream->codecpar, outputVideoCodecContext) < 0)
    {
        printf("Failed to copy encoder parameters to output stream \n");
        return -1;
    }
    outputVideoStream->time_base = outputVideoCodecContext->time_base;
    // Open output video file
    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, outputFilePath, AVIO_FLAG_WRITE) < 0) {
            printf("Failed to open output video file.\n");
            avformat_close_input(&formatContext);
            avcodec_free_context(&videoCodecContext);
            avcodec_free_context(&outputVideoCodecContext);
            av_frame_free(&frame);
            av_frame_free(&scaledFrame);
            av_free(buffer);
            sws_freeContext(scalerContext);
            avformat_free_context(outputFormatContext);
            return -1;
        }
    }

    if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        outputVideoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    // Write output file header
    if (avformat_write_header(outputFormatContext, NULL) < 0) {
        printf("Failed to write output file header.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&videoCodecContext);
        avcodec_free_context(&outputVideoCodecContext);
        av_frame_free(&frame);
        av_frame_free(&scaledFrame);
        av_free(buffer);
        sws_freeContext(scalerContext);
        avformat_free_context(outputFormatContext);
        return -1;
    }

    // Initialize frame and packet
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    // Initialize frame timestamp variables
    int64_t timestamp = 0;
    int64_t scaledTimestamp = 0;
    int64_t frameCnt = 0;
    bool stop = false;
    // Read frames from the input video file
    while (av_read_frame(formatContext, &packet) >= 0) {
        if (stop)
        {
            break;
        }
        
        AVStream *in_stream = formatContext->streams[packet.stream_index];
        // Check if the packet belongs to the video stream
        if (packet.stream_index == videoStreamIndex) {
            // Decode the video frame
            if (avcodec_send_packet(videoCodecContext, &packet) == 0) {
                while (avcodec_receive_frame(videoCodecContext, frame) == 0)
                {
                    printf("time_base{%d:%d}, packet pts:%lld, duration:%lld\n", videoCodecContext->time_base.num, videoCodecContext->time_base.den, frame->pts, frame->duration);
                    float nowtime = 1.f * frame->pts * videoCodecContext->time_base.num / videoCodecContext->time_base.den;
                    // Check if the frame is within the desired segment
                    if (nowtime <= segmentDuration)
                    {
                        printf("nowtime:%f\n", nowtime);
                        // Scale the frame
                        sws_scale(scalerContext, frame->data, frame->linesize, 0, videoCodecContext->height, scaledFrame->data, scaledFrame->linesize);
                        SaveAvFrame(scaledFrame);
                        scaledFrame->width = scaledWidth;
                        scaledFrame->height = scaledHeight;
                        scaledFrame->format = AV_PIX_FMT_YUV420P;
                        scaledFrame->pts = scaledTimestamp++;

                        // Encode and write the scaled frame to the output file
                        AVPacket outputPacket;
                        av_init_packet(&outputPacket);
                        outputPacket.data = NULL;
                        outputPacket.size = 0;
                        if (avcodec_send_frame(outputVideoCodecContext, scaledFrame) == 0) {
                            while (avcodec_receive_packet(outputVideoCodecContext, &outputPacket) == 0) {
                                printf("outputPacket ts:%lld, duration:%lld frameCnt:%d\n", outputPacket.pts, outputPacket.duration, frameCnt++);
                                outputPacket.stream_index = outputVideoStream->index;
                                outputPacket.duration = in_stream->time_base.den / in_stream->time_base.num / outputVideoStream->avg_frame_rate.num * outputVideoStream->avg_frame_rate.den;
                                av_packet_rescale_ts(&outputPacket, outputVideoCodecContext->time_base, outputVideoStream->time_base);
                                if (outputPacket.flags & AV_PKT_FLAG_KEY)
                                {
                                    printf("got key:%d, data:%02X %02X %02X %02X %02X\n", frameCnt, outputPacket.data[0], outputPacket.data[1], outputPacket.data[2], outputPacket.data[3], outputPacket.data[4]);
                                }
                                SaveH264(outputPacket.data, outputPacket.size);
                                int ret = av_interleaved_write_frame(outputFormatContext, &outputPacket);
                                if (ret < 0)
                                {
                                    fprintf(stderr, "Error during writing data to output file. "
                                                    "Error code: %s\n",
                                            av_err2str(ret));
                                    return ret;
                                }

                                av_packet_unref(&outputPacket);
                            }
                        }
                    }else{
                        stop = true;
                    }
                }
            }
        }
        else
        {
            if (streams_list[packet.stream_index] < 0)
            {
                av_packet_unref(&packet);
                continue;
            }
            packet.stream_index = streams_list[packet.stream_index];
            AVStream *out_stream = outputFormatContext->streams[packet.stream_index];
            /* copy packet */
            packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
            packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
            packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
            // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
            packet.pos = -1;

            if (av_interleaved_write_frame(outputFormatContext, &packet) < 0)
            {
                fprintf(stderr, "Error muxing packet\n");
                break;
            }
        }

        av_packet_unref(&packet);
    }

    // Write output file trailer
    av_write_trailer(outputFormatContext);

    // Close input video file
    avformat_close_input(&formatContext);

    // Free resources
    avcodec_free_context(&videoCodecContext);
    avcodec_free_context(&outputVideoCodecContext);
    av_frame_free(&frame);
    av_frame_free(&scaledFrame);
    av_free(buffer);
    sws_freeContext(scalerContext);
    /* close output */
    if (outputFormatContext && !(outputFormatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outputFormatContext->pb);
    avformat_free_context(outputFormatContext);

    fclose(fDump);
    fclose(f264Dump);
    printf("Video extraction and scaling completed successfully.\n");

    return 0;
}

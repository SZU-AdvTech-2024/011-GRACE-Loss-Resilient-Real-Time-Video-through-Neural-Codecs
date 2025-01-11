#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <stdio.h>
#include <jpeglib.h>

// 保存 RGB 图像为 JPEG 文件
int save_jpeg(const char *filename, AVFrame *frame, int width, int height) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *outfile;

    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return -1;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    int row_stride = width * 3;
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row_pointer = (JSAMPROW)(frame->data[0] + cinfo.next_scanline * row_stride);
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);

    return 0;
}

// 解码 BPG 文件为 JPEG
int decode_bpg_to_jpeg(const char *input_file, const char *output_file) {
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    struct SwsContext *sws_ctx = NULL;
    int video_stream_idx, ret;

    // 初始化 FFmpeg (已移除 av_register_all())

    // 打开 BPG 文件
    if (avformat_open_input(&fmt_ctx, input_file, NULL, NULL) < 0)
    {
        fprintf(stderr, "无法打开文件: %s\n", input_file);
        return -1;
    }

    // 查找流信息
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "无法找到流信息\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 查找视频流
    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (video_stream_idx < 0) {
        fprintf(stderr, "无法找到视频流\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 初始化解码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "无法分配解码器上下文\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar);

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "无法打开解码器\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 初始化帧和包
    frame = av_frame_alloc();
    packet = av_packet_alloc();

    // 读取帧
    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            ret = avcodec_send_packet(codec_ctx, packet);
            if (ret < 0) {
                fprintf(stderr, "发送包到解码器失败\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "解码帧失败\n");
                    goto cleanup;
                }

                // 转换为 RGB 格式
                AVFrame *rgb_frame = av_frame_alloc();
                int rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1);
                uint8_t *rgb_buffer = (uint8_t *)av_malloc(rgb_buffer_size);
                av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, rgb_buffer, AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1);

                // 使用 sws_scale 将 YUV 转换为 RGB
                sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                         codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
                                         SWS_BILINEAR, NULL, NULL, NULL);

                sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize,
                          0, codec_ctx->height, rgb_frame->data, rgb_frame->linesize);

                // 保存为 JPEG
                save_jpeg(output_file, rgb_frame, codec_ctx->width, codec_ctx->height);

                av_free(rgb_buffer);
                av_frame_free(&rgb_frame);
                goto cleanup; // 仅保存第一帧
            }
        }
        av_packet_unref(packet);
    }

cleanup:
    sws_freeContext(sws_ctx);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return 0;
}

int main() {
    const char *input_bpg = "output.bpg";
    const char *output_jpeg = "decoded_frame.jpg";

    if (decode_bpg_to_jpeg(input_bpg, output_jpeg) == 0) {
        printf("解码成功，保存为：%s\n", output_jpeg);
    } else {
        printf("解码失败\n");
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <x265.h>
#include <jpeglib.h>

#define WIDTH 2560  // 根据实际图像分辨率调整
#define HEIGHT 1440 // 根据实际图像分辨率调整

// 加载JPEG图像到内存
unsigned char* load_jpeg(const char *filename, int *width, int *height, int *channels) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *infile = fopen(filename, "rb");
    if (!infile) {
        perror("无法打开 JPEG 文件");
        return NULL;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *channels = cinfo.output_components;

    unsigned long data_size = (*width) * (*height) * (*channels);
    unsigned char *buffer = (unsigned char*)malloc(data_size);
    unsigned char *row_pointer = buffer;

    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, &row_pointer, 1);
        row_pointer += (*width) * (*channels);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return buffer;
}

// RGB 转 YUV420p 转换函数
void rgb_to_yuv420p(const unsigned char *rgb_data, uint8_t *yuv_data, int width, int height) {
    int y, u, v;
    int r, g, b;
    int i, j;

    // 处理 Y 分量
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            r = rgb_data[(i * width + j) * 3 + 0];
            g = rgb_data[(i * width + j) * 3 + 1];
            b = rgb_data[(i * width + j) * 3 + 2];

            // Y = 0.299 * R + 0.587 * G + 0.114 * B
            y = (int)(0.299 * r + 0.587 * g + 0.114 * b);
            y = y < 0 ? 0 : (y > 255 ? 255 : y); // 限制 Y 的范围

            // U = -0.14713 * R - 0.28886 * G + 0.436 * B
            u = (int)(-0.14713 * r - 0.28886 * g + 0.436 * b) + 128;
            u = u < 0 ? 0 : (u > 255 ? 255 : u); // 限制 U 的范围

            // V = 0.615 * R - 0.51499 * G - 0.10001 * B
            v = (int)(0.615 * r - 0.51499 * g - 0.10001 * b) + 128;
            v = v < 0 ? 0 : (v > 255 ? 255 : v); // 限制 V 的范围

            // 将 Y、U、V 存入 YUV 数据
            yuv_data[i * width + j] = (uint8_t)y;  // Y 分量
        }
    }

    // U 和 V 分量进行下采样 (4:2:0)
    int uv_width = width / 2;
    int uv_height = height / 2;

    for (i = 0; i < uv_height; i++) {
        for (j = 0; j < uv_width; j++) {
            r = rgb_data[(2 * i * width + 2 * j) * 3 + 0];
            g = rgb_data[(2 * i * width + 2 * j) * 3 + 1];
            b = rgb_data[(2 * i * width + 2 * j) * 3 + 2];

            u = (int)(-0.14713 * r - 0.28886 * g + 0.436 * b) + 128;
            u = u < 0 ? 0 : (u > 255 ? 255 : u);

            v = (int)(0.615 * r - 0.51499 * g - 0.10001 * b) + 128;
            v = v < 0 ? 0 : (v > 255 ? 255 : v);

            // 将 U 和 V 分量存入 YUV 数据
            yuv_data[width * height + i * uv_width + j] = (uint8_t)u;
            yuv_data[width * height + uv_width * uv_height + i * uv_width + j] = (uint8_t)v;
        }
    }
}

// 修改后的 encode_to_h265 函数
void encode_to_h265(const unsigned char *input_image, const char *output_file, int width, int height)
{
    unsigned char *yuv_data = (unsigned char *)malloc(width * height * 3 / 2); // YUV420p 格式
    rgb_to_yuv420p(input_image, yuv_data, width, height);  // RGB 转 YUV

    // 初始化 x265 编码器
    x265_param *params = x265_param_alloc();
    x265_param_default(params);
    params->sourceWidth = width;
    params->internalCsp = X265_CSP_I420;  // YUV420p
    params->sourceHeight = height;
    params->fpsNum = 30;
    params->fpsDenom = 1;
    params->internalBitDepth = 8;
    params->bRepeatHeaders = 1;
    params->rc.qp = 32;
    x265_encoder *encoder = x265_encoder_open(params);

    x265_picture *pic_in = x265_picture_alloc();
    x265_picture_init(params, pic_in);

    // Load image data into x265_picture structure
    pic_in->planes[0] = yuv_data;                // Y plane
    pic_in->planes[1] = yuv_data + width * height;  // U plane
    pic_in->planes[2] = pic_in->planes[1] + (width * height) / 4;  // V plane
    pic_in->stride[0] = width;
    pic_in->stride[1] = width / 2;
    pic_in->stride[2] = width / 2;

    // 编码并写入文件
    x265_nal *nal;
    uint32_t num_nals;
    int frame_size = x265_encoder_encode(encoder, &nal, &num_nals, pic_in, NULL);
    frame_size = x265_encoder_encode(encoder, &nal, &num_nals, NULL, NULL);
    FILE *out_file = fopen(output_file, "wb");
    if (!out_file) {
        perror("无法打开输出文件");
        x265_picture_free(pic_in);
        x265_encoder_close(encoder);
        x265_param_free(params);
        return;
    }
    if (frame_size < 0) {
        perror("Encoding failed");
        fclose(out_file);
        x265_picture_free(pic_in);
        x265_encoder_close(encoder);
        x265_param_free(params);
        return;
    }
    for (uint32_t i = 0; i < num_nals; i++) {
        if (fwrite(nal[i].payload, 1, nal[i].sizeBytes, out_file) != nal[i].sizeBytes) {
            perror("Failed to write NAL unit to file");
        }
    }

    fclose(out_file);
    x265_picture_free(pic_in);
    x265_encoder_close(encoder);
    x265_param_free(params);
}


// 构建BPG文件头并封装H.265码流
void create_bpg_file(const char *h265_file, const char *bpg_file) {
    FILE *infile = fopen(h265_file, "rb");
    FILE *outfile = fopen(bpg_file, "wb");

    // BPG文件头（固定的标识符）
    unsigned char bpg_header[] = {0x42, 0x50, 0x47, 0xFB};
    fwrite(bpg_header, 1, sizeof(bpg_header), outfile);

    // 读取并写入H.265码流
    unsigned char buffer[4096];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0) {
        fwrite(buffer, 1, read_bytes, outfile);
    }

    fclose(infile);
    fclose(outfile);
}

int main() {
    const char *input_jpeg = "glados.jpg";
    const char *temp_h265 = "temp.h265";
    const char *output_bpg = "output.bpg";

    int width, height, channels;
    unsigned char *image_data = load_jpeg(input_jpeg, &width, &height, &channels);
    if (!image_data) {
        return 1;
    }

    // 将JPEG图像编码为H.265
    encode_to_h265(image_data, temp_h265, width, height);

    // 创建BPG文件
    create_bpg_file(temp_h265, output_bpg);

    // 删除临时H.265文件
    remove(temp_h265);

    printf("编码完成，BPG文件保存为：%s\n", output_bpg);
    free(image_data);
    return 0;
}

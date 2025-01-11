#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <x265.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// 缓冲区存储编码结果
static uint8_t *encoded_data = NULL;
static size_t encoded_data_length = 0;

// BPG 编码函数
void bpg_encode_bytes(const uint8_t *frame, int height, int width) {
    // 创建编码器实例
    x265_param *params = x265_param_alloc();
    x265_param_default_preset(params, "ultrafast", "zerolatency");
    x265_encoder *encoder = NULL;

    params->internalCsp = X265_CSP_I420;  // 色彩空间
    params->sourceWidth = width;
    params->sourceHeight = height;
    params->fpsNum = 30;              // 帧率
    params->fpsDenom = 1;

    // 生成编码器实例
    encoder = x265_encoder_open(params);
    if (!encoder) {
        printf("Failed to open encoder\n");
        x265_param_free(params);
        return;
    }
    // 初始化图像数据结构
    x265_picture pic_in;
    x265_picture_init(params, &pic_in);

    // 设置 YUV 数据指针
    pic_in.planes[0] = (uint8_t*)frame;  // Y 分量
    pic_in.planes[1] = (uint8_t*)frame + width * height;  // U 分量
    pic_in.planes[2] = (uint8_t*)frame + width * height + (width / 2) * (height / 2);  // V 分量

    // 设置步长
    pic_in.stride[0] = width;
    pic_in.stride[1] = width / 2;
    pic_in.stride[2] = width / 2;

    // 编码
    x265_nal *nals = NULL;
    uint32_t num_nals = 0;
    int frame_size = x265_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_in);

    // 错误检查
    if (frame_size < 0) {
        printf("Error encoding frame\n");
    } else {
        printf("Encoded %d bytes\n", frame_size);

        // 计算编码后的数据总长度
        encoded_data_length = 0;
        for (uint32_t i = 0; i < num_nals; i++) {
            encoded_data_length += nals[i].sizeBytes;
        }

        // 为编码数据分配缓冲区
        encoded_data = (uint8_t *)malloc(encoded_data_length);
        if (encoded_data == NULL) {
            printf("Failed to allocate memory for encoded data\n");
            return;
        }

        // 将编码后的数据复制到缓冲区
        size_t offset = 0;
        for (uint32_t i = 0; i < num_nals; i++) {
            memcpy(encoded_data + offset, nals[i].payload, nals[i].sizeBytes);
            offset += nals[i].sizeBytes;
        }

        printf("Encoded data stored in buffer with size: %zu\n", encoded_data_length);
    }

    // 清理
    x265_encoder_close(encoder);
    x265_param_free(params);
}

// 获取编码后的缓冲区
uint8_t* get_buf() {
    return encoded_data;
}

// 获取编码后的数据长度
size_t get_buf_length() {
    return encoded_data_length;
}

// 释放内存
void free_memory(uint8_t *buf) {
    if (buf) {
        free(buf);
    }
    encoded_data = NULL;
    encoded_data_length = 0;
}

// 读取图像并将其传递给编码器
int load_and_encode_image(const char *image_path) {
    int width, height, channels;
    uint8_t *img_data = stbi_load(image_path, &width, &height, &channels, 3); // 只加载 RGB 图像

    if (img_data == NULL) {
        printf("Error loading image %s\n", image_path);
        return -1;
    }

    printf("Loaded image: %s\n", image_path);
    printf("Image dimensions: %dx%d, channels: %d\n", width, height, channels);

    // 调用编码函数
    bpg_encode_bytes(img_data, height, width);

    // 获取编码后的缓冲区
    size_t buflen = get_buf_length();
    uint8_t *encoded = get_buf();
    printf("Encoded stream size: %zu bytes\n", buflen);

    // 释放图像内存
    stbi_image_free(img_data);

    // 释放编码后的缓冲区内存
    free_memory(encoded);

    return 0;
}

int main() {
    const char *image_path = "glados.jpg";  // 替换为你的图像路径
    if (load_and_encode_image(image_path) != 0) {
        return -1;
    }

    return 0;
}

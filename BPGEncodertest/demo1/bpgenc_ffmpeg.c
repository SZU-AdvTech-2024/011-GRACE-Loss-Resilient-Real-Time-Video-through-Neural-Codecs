#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void execute_command(const char *command) {
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "命令执行失败: %s\n", command);
        exit(1);
    }
}

int main() {
    const char *input_image = "glados.png";
    const char *output_bpg = "output.bpg";

    // 使用ffmpeg将输入图像直接编码为H.265格式
    execute_command("ffmpeg -i glados.jpg -pix_fmt yuv420p -c:v libx265 -preset slow -x265-params lossless=1 temp.h265");

    // 创建BPG文件并写入头信息
    FILE *bpg_file = fopen(output_bpg, "wb");
    if (!bpg_file) {
        perror("无法创建BPG文件");
        return 1;
    }

    // 写入BPG头
    unsigned char bpg_header[] = {0x42, 0x50, 0x47, 0xFB}; // "BPG\xFB"
    fwrite(bpg_header, 1, sizeof(bpg_header), bpg_file);

    // 写入H.265码流
    FILE *h265_file = fopen("temp.h265", "rb");
    if (!h265_file) {
        perror("无法打开H.265文件");
        fclose(bpg_file);
        return 1;
    }
    unsigned char buffer[4096];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), h265_file)) > 0) {
        fwrite(buffer, 1, read_bytes, bpg_file);
    }

    fclose(h265_file);
    fclose(bpg_file);

    // 删除临时文件
    execute_command("rm temp.h265");

    printf("编码完成，BPG文件保存为：%s\n", output_bpg);
    return 0;
}

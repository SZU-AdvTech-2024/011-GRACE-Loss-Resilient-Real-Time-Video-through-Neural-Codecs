#include <stdio.h>
#include <stdlib.h>

void execute_command(const char *command) {
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "命令执行失败: %s\n", command);
        exit(1);
    }
}

int main() {
    const char *input_bpg = "output.bpg";
    const char *output_image = "decoded.png";

    // 提取H.265码流
    FILE *bpg_file = fopen(input_bpg, "rb");
    if (!bpg_file) {
        perror("无法打开BPG文件");
        return 1;
    }

    // 跳过BPG头（4字节）
    fseek(bpg_file, 4, SEEK_SET);

    // 将剩余数据写入H.265文件
    FILE *h265_file = fopen("temp_decoded.h265", "wb");
    if (!h265_file) {
        perror("无法创建H.265文件");
        fclose(bpg_file);
        return 1;
    }

    unsigned char buffer[4096];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), bpg_file)) > 0) {
        fwrite(buffer, 1, read_bytes, h265_file);
    }

    fclose(bpg_file);
    fclose(h265_file);

    // 使用ffmpeg解码H.265文件并保存为PNG
    execute_command("ffmpeg -i temp_decoded.h265 -pix_fmt rgb24 -frames:v 1 decoded.jpg");

    // 删除临时文件
    execute_command("rm temp_decoded.h265");

    printf("解码完成，图像保存为：%s\n", output_image);
    return 0;
}

import ctypes
import numpy as np
import torch
from PIL import Image  # 用于保存图像

# 加载共享库
lib = ctypes.CDLL("./bpgenc.so")
lib2 = ctypes.CDLL("./bpgdec.so")

# 定义函数原型
bpg_encode_bytes = lib.bpg_encode_bytes
bpg_encode_bytes.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int, ctypes.c_int]

get_buf = lib.get_buf
get_buf.restype = ctypes.POINTER(ctypes.c_char)

get_buf_length = lib.get_buf_length
get_buf_length.restype = ctypes.c_size_t

free_mem = lib.free_memory

bpg_decode_bytes = lib2.bpg_decode_bytes
bpg_decode_bytes.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t, ctypes.c_int, ctypes.c_int]
bpg_decode_bytes.restype = ctypes.POINTER(ctypes.c_ubyte)

free_dec_mem = lib2.free_memory

# 读取图像并转换为 YUV 格式
def read_image_as_yuv(filepath):
    from PIL import Image
    img = Image.open(filepath).convert("RGB")  # 读取并转换为 RGB
    width, height = img.size  # 获取原始宽高

    img_np = np.array(img).astype(np.uint8)  # 转为 NumPy 数组 (H, W, 3)
    h, w, _ = img_np.shape

    # YUV420 数据大小
    yuv = np.empty((h * w * 3 // 2,), dtype=np.uint8)

    # 计算 Y 分量并展平
    yuv[: h * w] = (
        0.299 * img_np[:, :, 0] + 0.587 * img_np[:, :, 1] + 0.114 * img_np[:, :, 2]
    ).flatten()

    # 计算 U 分量 (抽样后展平)
    yuv[h * w : h * w * 5 // 4] = img_np[::2, ::2, 1].flatten()

    # 计算 V 分量 (抽样后展平)
    yuv[h * w * 5 // 4 :] = img_np[::2, ::2, 2].flatten()

    return yuv, height, width



# 保存解码后的图像
def save_decoded_image(decoded_bytes, h, w, output_path="output.png"):
    # 转换为 PIL 图像
    img_array = np.frombuffer(decoded_bytes, dtype=np.uint8).reshape((h, w, 3))
    img = Image.fromarray(img_array, "RGB")
    img.save(output_path)
    print(f"Image saved as {output_path}")

# 编解码测试
def main():
    frame, h, w = read_image_as_yuv("glados.jpg")
    frame_bytes = (ctypes.c_ubyte * len(frame)).from_buffer(bytearray(frame))

    # 编码
    bpg_encode_bytes(frame_bytes, h, w)
    encoded_length = get_buf_length()
    encoded_data = ctypes.string_at(get_buf(), encoded_length)
    print(f"Encoded size: {encoded_length} bytes")

    # 解码
    bpg_stream = (ctypes.c_ubyte * len(encoded_data)).from_buffer_copy(encoded_data)
    decoded_ptr = bpg_decode_bytes(bpg_stream, len(encoded_data), h, w)
    decoded_bytes = ctypes.string_at(decoded_ptr, h * w * 3)
    print(f"Decoded size: {len(decoded_bytes)} bytes")

    # 保存解码后的图像
    save_decoded_image(decoded_bytes, h, w, output_path="output.png")

    # 清理内存
    free_mem()
    free_dec_mem(decoded_ptr)

if __name__ == "__main__":
    main()

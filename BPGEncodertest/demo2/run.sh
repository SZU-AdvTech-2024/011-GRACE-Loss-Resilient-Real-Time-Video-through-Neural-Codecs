ffmpeg -i glados.jpg -c:v libx265 -preset fast -x265-params log-level=error -pix_fmt yuvj420p -f rawvideo output.bpg

ffmpeg -i output.bpg -f image2 -pix_fmt yuvj420p output_decoded.jpg

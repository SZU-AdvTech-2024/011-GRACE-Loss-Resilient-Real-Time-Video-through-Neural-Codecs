# GRACE: Loss-Resilient Real-Time Video through Neural Codecs

## Important License Information 原许可信息

Before using any software or code from this repository, please refer to the LICENSE file for detailed terms and conditions. It is crucial to understand that while the majority of the code in this repository is governed by the Academic Software License © 2023 UChicago

Additionally, please note that the content within the following specific subfolders and files ("baselines/error_concealment", "grace/subnet", and "grace/net.py") did not originate from the authors and may be subject to different terms and conditions, not covered under the provided license. 

## 安装依赖

```bash
sudo apt install ffmpeg ninja-build # install the ffmpeg and ninja
conda env create -f env.yml # creating the conda environment
```

## 预训练模型与数据集

数据集链接: https://drive.google.com/file/d/1iQhTfb7Kew_z97kDVoj2vOmQqaNjBK9J/view?usp=sharing

预训练模型链接: https://drive.google.com/file/d/1IWD-VUc0RPXXhBzoH5j9YD6bl8kzYYJ1/view?usp=sharing

```bash
cp /your/download/path/grace_models.tar
cd models/
tar xzvf grace_models.tar 

cd ../videos/
cp /your/download/path/GraceVideos.zip .
unzip GraceVideos.zip
```

## 训练

```bash
# activate the conda env
conda activate grace-test

# run Grace
LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 grace-gpu.py

# run pretrained AE models
LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 pretrained-gpu.py

# run h265/h264 baseline
# NOTE: after running this, you may have to restart your terminal
LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 h26x.py

# run error concealment
# WARNING: this could take a long time (couple of hours)
LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 error-concealment.py
```

## 结果绘图

```bash
cd results/
python3 plot.py
```


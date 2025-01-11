cd /home/wangyihui/code/Grace

# activate the conda env
conda activate grace-test

# run Grace
CUDA_VISIBLE_DEVICES=7 LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 grace-gpu.py

# run pretrained AE models
CUDA_VISIBLE_DEVICES=7 LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 pretrained-gpu.py

# run h265/h264 baseline
# NOTE: after running this, you may have to restart your terminal
CUDA_VISIBLE_DEVICES=6 LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 h26x.py

# run error concealment
# WARNING: this could take a long time (couple of hours)
CUDA_VISIBLE_DEVICES=6 LD_LIBRARY_PATH=libs/:${LD_LIBRARY_PATH} python3 error-concealment.py
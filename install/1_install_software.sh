#!/usr/bin/env bash
set -xe

sudo apt purge -y \
    $(apt search linux-image-5.4    | grep installed | awk -F/ '{print $1}') \
    $(apt search linux-modules-5.4  | grep installed | awk -F/ '{print $1}') \
    $(apt search linux-image-5.15   | grep installed | awk -F/ '{print $1}') \
    $(apt search linux-modules-5.15 | grep installed | awk -F/ '{print $1}')
sudo apt autoremove -y

curl -fsSL https://get.docker.com | sh

sudo groupadd docker || true
sudo usermod -aG docker $USER
 
wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/xrt_202110.2.11.691_20.04-amd64-xrt.deb" -O /tmp/xrt_202110.2.11.691_20.04-amd64-xrt.deb
sudo apt install -y /tmp/xrt_202110.2.11.691_20.04-amd64-xrt.deb


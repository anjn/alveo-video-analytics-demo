## Install on Ubuntu 20.04

### (Optional) Enable remote ssh access
```bash
$ sudo apt install -y openssh-server
```

### (Optional) Install favorite editor
```bash
$ sudo apt install -y vim-nox
```

### Update OS
```bash
$ sudo apt update
$ sudo apt upgrade -y
$ sudo reboot
```

### Install utilities
```bash
$ sudo apt install -y curl git tmux htop
```

### Downgrade Linux kernel from 5.15 to 5.11
Check kernel version
```bash
$ uname -r
5.15.0-52-generic
```

Install kernel 5.11
```bash
$ sudo apt install \
    linux-image-5.11.0-46-generic \
    linux-modules-5.11.0-46-generic \
    linux-headers-5.11.0-46-generic \
    linux-modules-extra-5.11.0-46-generic \
    linux-hwe-5.11-headers-5.11.0-46
```

List boot options
```bash
$ grep -e '\(menuentry\|submenu\) ' /boot/grub/grub.cfg | awk -F\' '{print $1 $2}'
menuentry Ubuntu
submenu Advanced options for Ubuntu
        menuentry Ubuntu, with Linux 5.15.0-52-generic
        menuentry Ubuntu, with Linux 5.15.0-52-generic (recovery mode)
        menuentry Ubuntu, with Linux 5.11.0-46-generic
        menuentry Ubuntu, with Linux 5.11.0-46-generic (recovery mode)
        menuentry Ubuntu, with Linux 5.4.0-42-generic
        menuentry Ubuntu, with Linux 5.4.0-42-generic (recovery mode)
menuentry UEFI Firmware Settings
```

Select kernel to boot
```bash
$ sudo grub-set-default 'Advanced options for Ubuntu>Ubuntu, with Linux 5.11.0-46-generic'
$ sudo sed -i "s/^GRUB_DEFAULT=.*\$/GRUB_DEFAULT=saved/" /etc/default/grub
$ sudo update-grub2
$ sudo reboot
```

Check kernel version
```bash
$ uname -r
5.11.0-46-generic
```

Uninstall kernels that are not supported by XRT 2021.1
```bash
$ sudo apt purge \
    $(apt search linux-image-5.15 | grep installed | awk -F/ '{print $1}') \
    $(apt search linux-modules-5.15 | grep installed | awk -F/ '{print $1}')
$ sudo apt autoremove
```

### Install Docker
```bash
$ curl -fsSL https://get.docker.com | sh
```
Enable docker for a non-root user
```bash
$ sudo groupadd docker
$ sudo usermod -aG docker $USER
$ newgrp docker
```

### Install XRT 2021.1 (2.11.691)
```bash
$ wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/xrt_202110.2.11.691_20.04-amd64-xrt.deb" -O xrt_202110.2.11.691_20.04-amd64-xrt.deb
$ sudo apt install -y ./xrt_202110.2.11.691_20.04-amd64-xrt.deb
```

### Install Alveo platforms
Install U50LV platform
```bash
$ wget "https://www.xilinx.com/bin/public/openDownload?filename=xilinx-u50lv-gen3x4-xdma-platform_2-2_all.deb.tar.gz" -O xilinx-u50lv-gen3x4-xdma-platform_2-2_all.deb.tar.gz
$ mkdir u50lv
$ tar xf xilinx-u50lv-gen3x4-xdma-platform_2-2_all.deb.tar.gz -C u50lv
$ sudo apt install -y ./u50lv/*.deb
```
Downgrade SC FW of U50LV
```bash
$ wget "https://www.xilinx.com/bin/public/openDownload?filename=xilinx-u50-gen3x16-xdma-all_1-2784799.deb.tar.gz" -O xilinx-u50-gen3x16-xdma-all_1-2784799.deb.tar.gz
$ mkdir u50
$ tar xf xilinx-u50-gen3x16-xdma-all_1-2784799.deb.tar.gz -C u50
$ sudo apt install -y --allow-downgrades ./u50/xilinx-sc-fw-u50_5.2.6-2.eef518f_all.deb
```

Install U55C platform
```bash
$ wget "https://www.xilinx.com/bin/public/openDownload?filename=xilinx-u55c-gen3x16-xdma-platform_2-3_all.deb.tar.gz" -O xilinx-u55c-gen3x16-xdma-platform_2-3_all.deb.tar.gz
$ mkdir u55c
$ tar xf xilinx-u55c-gen3x16-xdma-platform_2-3_all.deb.tar.gz -C u55c
$ sudo apt install -y ./u55c/*.deb
```

Install U30 platform
```bash
$ mkdir u30
$ wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/raptor_packages/xilinx-sc-fw-u30_6.3.8-1.cd35f69_all.deb" -O u30/xilinx-sc-fw-u30_6.3.8-1.cd35f69_all.deb
$ wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/raptor_packages/xilinx-u30-gen3x4-base_2-3391496_all.deb" -O u30/xilinx-u30-gen3x4-base_2-3391496_all.deb
$ wget "https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/raptor_packages/xilinx-u30-gen3x4-validate_2-3380610_all.deb" -O u30/xilinx-u30-gen3x4-validate_2-3380610_all.deb
$ sudo apt install -y ./u30/*.deb
```
Reboot
```bash
$ sudo reboot
```

### Check Alveo platforms
```bash
$ /opt/xilinx/xrt/bin/xbmgmt examine
System Configuration
  OS Name              : Linux
  Release              : 5.11.0-46-generic
  Version              : #51~20.04.1-Ubuntu SMP Fri Jan 7 06:51:40 UTC 2022
  Machine              : x86_64
  CPU Cores            : 24
  Memory               : 128741 MB
  Distribution         : Ubuntu 20.04.5 LTS
  GLIBC                : 2.31
  Model                : System Product Name

XRT
  Version              : 2.11.691
  Branch               : 2021.1
  Hash                 : 3e695ed86d15164e36267fb83def6ff2aaecd758
  Hash Date            : 2021-11-18 18:16:26
  XOCL                 : 2.11.691, 3e695ed86d15164e36267fb83def6ff2aaecd758
  XCLMGMT              : 2.11.691, 3e695ed86d15164e36267fb83def6ff2aaecd758

Devices present
  [0000:0a:00.0] : xilinx_u30_gen3x4_base_2
  [0000:09:00.0] : xilinx_u30_gen3x4_base_2
  [0000:08:00.0] : xilinx_u50lv_gen3x4_xdma_base_2
```

### (Optional) Flash cards
**CAUTION** When flashing U50LV/U55C, remove U30 from the host to prevent bricking.

```bash
$ sudo /opt/xilinx/xrt/bin/xbmgmt program --base --device 08:00.0
$ sudo /opt/xilinx/xrt/bin/xbmgmt program --base --device 09:00.0
$ sudo /opt/xilinx/xrt/bin/xbmgmt program --base --device 0a:00.0
$ sudo reboot
```

### Check U50LV's core voltage
Confirm that `Internal FPGA Vcc` is 0.72V
```bash
$ /opt/xilinx/xrt/bin/xbutil examine --device 08:00.1 --report electrical

-----------------------------------------------------
1/1 [0000:08:00.1] : xilinx_u50lv_gen3x4_xdma_base_2
-----------------------------------------------------
Electrical
  Max Power              : 75 Watts
  Power                  : 5.979788 Watts
  Power Warning          : false

  Power Rails            : Voltage   Current
  12 Volts PCI Express   : 12.010 V,  0.490 A
  3.3 Volts PCI Express  :  3.272 V,  0.029 A
  Internal FPGA Vcc      :  0.719 V,  1.300 A
  Internal FPGA Vcc IO   :  0.850 V,  0.500 A
  5.5 Volts System       :  5.016 V
  1.8 Volts Top          :  1.806 V
  0.9 Volts Vcc          :  0.901 V
  Mgt Vtt                :  1.200 V
  3.3 Volts Vcc          :  3.333 V
  1.2 Volts HBM          :  1.201 V
  Vpp 2.5 Volts          :  2.516 V
```

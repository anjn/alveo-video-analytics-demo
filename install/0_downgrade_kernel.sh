#!/usr/bin/env bash
set -xe

sudo apt update
sudo apt upgrade -y

sudo apt install \
    linux-image-5.11.0-46-generic \
    linux-modules-5.11.0-46-generic \
    linux-headers-5.11.0-46-generic \
    linux-modules-extra-5.11.0-46-generic \
    linux-hwe-5.11-headers-5.11.0-46

sudo grub-set-default 'Advanced options for Ubuntu>Ubuntu, with Linux 5.11.0-46-generic'
sudo sed -i "s/^GRUB_DEFAULT=.*\$/GRUB_DEFAULT=saved/" /etc/default/grub
sudo update-grub2

sudo reboot

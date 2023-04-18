#!/bin/bash
#
uid=$(stat -c "%u" .)
gid=$(stat -c "%g" .)

getent group $USER || groupadd --gid $gid $USER
#id $USER &> /dev/null || useradd -M $USER --uid $uid --gid $gid
id $USER &> /dev/null || useradd -m $USER --uid $uid --gid $gid

echo "%$USER    ALL=(ALL)   NOPASSWD:    ALL" > /etc/sudoers.d/$USER
chmod 0440 /etc/sudoers.d/$USER

export USER=$USER
export HOME=/home/$USER
export LOGNAME=$USER
export PATH=/usr/local/bin:/bin:/usr/bin

exec setpriv --reuid=$USER --regid=$USER --init-groups "$@"

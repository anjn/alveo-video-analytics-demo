#!/usr/bin/env bash
set -e

dir=$(dirname $(readlink -f $0))

# Local files
videos+=($(find $dir/videos -name "*.ts" | sort))

max_videos=32
num_videos=${#videos[@]}

if [[ $num_videos -eq 0 ]] ; then
    echo "Error: Couldn't find videos!"
    exit 1
fi

if [[ $num_videos -gt $max_videos ]] ; then
    num_videos=$max_videos
fi

# Create config file
sed "s/%HOST_SERVER_IP%/$HOST_SERVER_IP/" ./rtsp-simple-server.yml > /tmp/rtsp-simple-server.yml

/usr/local/bin/rtsp-simple-server /tmp/rtsp-simple-server.yml &
sleep 1

for i in $(seq 0 $((num_videos-1))) ; do
    port=8554
    name=test$(printf %02x $i)
    rtsp=rtsp://localhost:$port/$name
    v=${videos[i%num_videos]}
    echo "Info: Start streaming [$rtsp] $v"
    ffmpeg -re -stream_loop -1 -i "$v" -c copy -bsf:v 'filter_units=pass_types=1-5' -f rtsp $rtsp &> /dev/null &
    sleep 0.2
done

wait


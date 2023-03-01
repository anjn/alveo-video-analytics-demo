#!/usr/bin/env bash
set -e

dir=$(dirname $(readlink -f $0))

cd $dir

tmux new-session -d -s demo-session "$dir/docker/run.sh --name demo-ml --port 8554,5555,5556 --card u5 ./start_ml.sh"
tmux select-layout even-horizontal

tmux split-window -fv
tmux send "$dir/docker/enter.sh --name demo-ml ./start_rtsp.sh" ENTER

ip_ml=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' demo-ml)

tmux split-window -fv
tmux send "$dir/docker/run.sh --name demo-video --port 8555 --env ML_SERVER_IP=$ip_ml,RTSP_SERVER_IP=$ip_ml --card u30 ./start_demo.sh" ENTER

tmux a


#!/usr/bin/env bash
set -e

dir=$(dirname $(readlink -f $0))

tmux new-session -d -s demo-session "$dir/docker/run.sh ./start_ml.sh"
tmux select-layout even-horizontal
tmux split-window -fv
sleep 1
tmux send "$dir/docker/enter.sh ./start_rtsp.sh" ENTER
tmux split-window -fv
tmux send "$dir/docker/enter.sh ./start_demo.sh" ENTER
tmux a


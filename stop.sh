#!/usr/bin/env bash

docker rm -f alveo-video-analytics-demo

tmux kill-session -t demo-session


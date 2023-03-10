#!/usr/bin/env bash

docker rm -f demo-video demo-ml
docker-compose down

tmux kill-session -t demo-session


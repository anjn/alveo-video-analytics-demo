#!/usr/bin/env bash

command=/bin/bash
if [[ $# -gt 0 ]] ; then
    command="$*"
fi

docker exec -it alveo-video-analytics-demo $command


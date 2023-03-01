#!/usr/bin/env bash

VALID_ARGS=$(getopt -o h --long help,name: -- "$@")
if [[ $? -ne 0 ]]; then
    exit 1;
fi

name=demo-video

eval set -- "$VALID_ARGS"
while [ : ]; do
  case "$1" in
    --name)
        name=$2 ; shift 2 ;;
    -h | --help)
        echo "$0 [-h|--help] [--name NAME]" ; exit ;;
    --)
        shift ; break ;;
  esac
done

command=/bin/bash
if [[ $# -gt 0 ]] ; then
    command="$*"
fi

docker exec -it $name $command


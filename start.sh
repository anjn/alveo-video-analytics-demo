#!/usr/bin/env bash
dir=$(dirname $(readlink -f $0))

cd $dir

$dir/stop.sh &> /dev/null

upsec=$(awk '{print int($1)}' /proc/uptime)
until [[ $upsec -gt 90 ]] ; do
    echo "Waiting for devices get ready ($upsec)"
    upsec=$(awk '{print int($1)}' /proc/uptime)
    sleep 2
done

docker-compose up -d

if [[ -z $HOST_SERVER_IP ]] ; then
    HOST_SERVER_IP=$(ip route get 1.2.3.4 | head -n 1 | awk '{print $7}')
fi

tmux new-session -d -s demo-session "$dir/docker/run.sh --name demo-ml --port 8554,5555,5556,8888,8889,8189/udp --env HOST_SERVER_IP=$HOST_SERVER_IP --card u5 ./start_ml.sh"
tmux select-layout even-horizontal

ip_ml=
until [[ -n $ip_ml ]] ; do
    sleep 1
    ip_ml=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' demo-ml || true)
done

tmux split-window -fv
tmux send "$dir/docker/enter.sh --name demo-ml ./start_rtsp.sh" ENTER

tmux split-window -fv
tmux send "$dir/docker/run.sh --name demo-video --port 8555 --env ML_SERVER_IP=$ip_ml,RTSP_SERVER_IP=$ip_ml,HOST_SERVER_IP=$HOST_SERVER_IP --card u30 ./start_demo.sh" ENTER

tmux a


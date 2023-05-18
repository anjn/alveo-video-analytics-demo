#!/usr/bin/env bash
dir=$(dirname $(readlink -f $0))

source $dir/docker/env.sh

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

tmux new-session -d -s demo-session "$dir/docker/run.sh --name demo-ml --port 8554,5555,5556,5557,8888,8889,8189/udp --env HOST_SERVER_IP=$HOST_SERVER_IP --card $card_ml ./start_ml.sh"
tmux select-layout even-horizontal

ip_ml=
sleep 2
until [[ -n $ip_ml ]] ; do
    echo Waiting for ML container up
    sleep 1
    ip_ml=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' demo-ml || true)
done

tmux split-window -fv
tmux send "$dir/docker/enter.sh --name demo-ml ./start_rtsp.sh" ENTER

tmux split-window -fv
tmux send "$dir/docker/run.sh --name demo-video --port 8555 --env ML_SERVER_IP=$ip_ml,RTSP_SERVER_IP=$ip_ml,HOST_SERVER_IP=$HOST_SERVER_IP --card $card_video ./start_demo.sh" ENTER

# Wait Grafana up
echo Waiting for Grafana up
until nc -z localhost 3000 ; do
    sleep 1
done
sleep 5

if [[ -z $(which jq) ]] ; then
    set -x
    sudo apt install -y jq
    set +x
fi

# Add influxdb datasource to Grafana
curl \
    "http://localhost:3000/api/datasources" \
    -X POST \
    -H "Content-Type: application/json" \
    --user admin:admin \
    --data-binary @<(sed "s/%HOST_SERVER_IP%/$HOST_SERVER_IP/" ./docker/grafana/datasource/influxdb.json)

# Get datasource uid
DS_UID=$(curl -s "http://localhost:3000/api/datasources" -u admin:admin | jq -r ".[0].uid")

# Add dashboard
curl \
    "http://localhost:3000/api/dashboards/db" \
    -X POST \
    -H "Content-Type: application/json" \
    --user admin:admin \
    --data-binary @<(sed "s/%HOST_SERVER_IP%/$HOST_SERVER_IP/" ./docker/grafana/dashboard.json | sed "s/%DS_UID%/$DS_UID/")

tmux a


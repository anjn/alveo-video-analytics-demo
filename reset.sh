./docker/run.sh -- /opt/xilinx/xrt/bin/xbutil reset -d --force
./docker/run.sh -- /opt/xilinx/xrt/bin/xbutil program -d -u /opt/xilinx/xclbin/v70.xclbin

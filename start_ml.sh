#!/usr/bin/bash

#if /opt/xilinx/xrt/bin/xbutil examine | grep u55c > /dev/null ; then
#    export XLNX_VART_FIRMWARE=/opt/xilinx/overlaybins/DPUCAHX8H/u55cdwc/dpu_DPUCAHX8H_DWC_11PE300_xilinx_u55c_gen3x16_xdma_base_2.xclbin
#    max_batch_size=3
#fi
#
#if /opt/xilinx/xrt/bin/xbutil examine | grep u50lv > /dev/null ; then
#    export XLNX_VART_FIRMWARE=/opt/xilinx/overlaybins/DPUCAHX8H/u50lvdwc/dpu_DPUCAHX8H_DWC_8PE275_xilinx_u50lv_gen3x4_xdma_base_2.xclbin
#    max_batch_size=4
#fi
#
#LD_LIBRARY_PATH=/opt/xilinx/xrt/lib /opt/xilinx/xrm/bin/xrmd > /dev/null &

source /opt/xilinx/xrt/setup.sh
source /opt/xilinx/xrm/setup.sh
echo -e "[Runtime]\nxgq_polling=true" | sudo tee /opt/xilinx/xrt/xrt.ini
export XRT_INI_PATH=/opt/xilinx/xrt/xrt.ini
export XCLBIN_PATH=/opt/xilinx/xclbin
export XLNX_VART_FIRMWARE=/opt/xilinx/xclbin/v70.xclbin
export XLNX_ENABLE_DEVICES=0
export XLNX_ENABLE_FINGERPRINT_CHECK=0

xrmd &

sudo cp -rf /workspace/demo/docker/packages/librt-engine.so /usr/lib/librt-engine.so

sudo mkdir -p /usr/share/vitis_ai_library/models
sudo cp -rf /workspace/demo/docker/models/yolov3_voc_tf /usr/share/vitis_ai_library/models/
sudo cp -rf /workspace/demo/docker/models/chen_color_resnet18_pt /usr/share/vitis_ai_library/models/
sudo cp -rf /workspace/demo/docker/models/vehicle_* /usr/share/vitis_ai_library/models/
sudo cp -rf /opt/xilinx/examples/yolov3_label.json /usr/share/vitis_ai_library/models/yolov3_voc_tf/label.json

#mkdir -p /tmp/build
#pushd /tmp/build
mkdir -p build
pushd build
#cmake -DCMAKE_BUILD_TYPE=Debug /workspace/demo
#make -j$(nproc) VERBOSE=1 --no-print-directory
cmake /workspace/demo
make -j$(nproc) ml_server

# Copy config file
cp /workspace/demo/config.toml .
./ml_server #--max-batch-size $max_batch_size


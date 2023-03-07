#!/usr/bin/bash

if /opt/xilinx/xrt/bin/xbutil examine | grep u55c > /dev/null ; then
    export XLNX_VART_FIRMWARE=/opt/xilinx/overlaybins/DPUCAHX8H/u55cdwc/dpu_DPUCAHX8H_DWC_11PE300_xilinx_u55c_gen3x16_xdma_base_2.xclbin
    max_batch_size=3
fi

if /opt/xilinx/xrt/bin/xbutil examine | grep u50lv > /dev/null ; then
    export XLNX_VART_FIRMWARE=/opt/xilinx/overlaybins/DPUCAHX8H/u50lvdwc/dpu_DPUCAHX8H_DWC_8PE275_xilinx_u50lv_gen3x4_xdma_base_2.xclbin
    max_batch_size=4
fi

LD_LIBRARY_PATH=/opt/xilinx/xrt/lib /opt/xilinx/xrm/bin/xrmd > /dev/null &

mkdir -p /tmp/build
pushd /tmp/build
#cmake -DCMAKE_BUILD_TYPE=Debug /workspace/demo
#make -j$(nproc) VERBOSE=1 --no-print-directory
cmake /workspace/demo
make -j$(nproc) ml_server
popd

/tmp/build/ml_server --max-batch-size $max_batch_size


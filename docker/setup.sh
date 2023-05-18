if [[ $CARD_VIDEO == v70 ]] ; then

    cd /workspace/demo
    source /opt/xilinx/xrt/setup.sh
    source /opt/xilinx/xrm/setup.sh
    echo -e "[Runtime]\nxgq_polling=true" | sudo tee /opt/xilinx/xrt/xrt.ini
    export XRT_INI_PATH=/opt/xilinx/xrt/xrt.ini
    export XCLBIN_PATH=/opt/xilinx/xclbin
    export XLNX_VART_FIRMWARE=/opt/xilinx/xclbin/v70.xclbin
    export XLNX_ENABLE_DEVICES=0
    export XLNX_ENABLE_FINGERPRINT_CHECK=0
    
    xrmd &
    source /opt/xilinx/vvas/setup.sh
    export VVAS_CORE_LOG_FILE_PATH=CONSOLE
    #Needed for RawTensor graph runner multi threaded use case 
    sudo cp -rf /workspace/demo/docker/packages/librt-engine.so /usr/lib/librt-engine.so
    sudo cp -rf /opt/xilinx/xclbin/image_processing.cfg /opt/xilinx/vvas/share
    #Copy the models required by Vitis AI
    sudo mkdir -p /usr/share/vitis_ai_library/models
    sudo cp -rf /workspace/demo/docker/models/yolov3_voc_tf /usr/share/vitis_ai_library/models/
    sudo cp -rf /workspace/demo/docker/models/yolov6m_pt /usr/share/vitis_ai_library/models/
    sudo cp -rf /workspace/demo/docker/models/chen_color_resnet18_pt /usr/share/vitis_ai_library/models/
    sudo cp -rf /workspace/demo/docker/models/vehicle_* /usr/share/vitis_ai_library/models/
    sudo cp -rf /opt/xilinx/examples/yolov3_label.json /usr/share/vitis_ai_library/models/yolov3_voc_tf/label.json
    
    sudo cp -rf /workspace/demo/docker/models/custom_car_color /usr/share/vitis_ai_library/models/
    sudo cp -rf /workspace/demo/docker/models/custom_car_type /usr/share/vitis_ai_library/models/
    
    sudo apt install -y --allow-downgrades /workspace/demo/docker/packages/libvitis_ai_library_3.0.0_amd64.deb

elif [[ $CARD_VIDEO == u30 ]] ; then

    # Count available device
    export max_devices=$(/opt/xilinx/xrt/bin/xbutil examine | grep u30 | wc -l)
    
    # Setup Video SDK
    export GST_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0
    LD_LIBRARY_PATH=/opt/xilinx/xrt/lib /opt/xilinx/xrm/bin/xrmd > /dev/null &
    source /opt/xilinx/xcdr/setup.sh

fi


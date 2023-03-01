FROM ubuntu:20.04

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive

# Timezone
ENV TZ=Asia/Tokyo
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Clean cache
#RUN --mount=type=cache,target=/var/lib/apt/lists \
#    --mount=type=cache,target=/var/cache/apt/archives \
#    rm -rf /var/lib/apt/lists/* \
#    && apt-get clean

# Clean cache
#RUN --mount=type=cache,target=/root/downloads \
#    rm -rf /root/downloads/*

RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update \
    && apt-get install -y --no-install-recommends \
        apt-transport-https \
        autoconf \
        automake \
        build-essential \
        ca-certificates \
        checkinstall \
        cmake \
        curl \
        git \
        libeigen3-dev \
        libgoogle-glog-dev \
        libgtest-dev \
        libopencv-dev \
        libssl-dev \
        libtool \
        net-tools \
        pkg-config \
        python3-dev \
        python3-pip \
        python3-setuptools \
        python3-wheel \
        unzip \
        wget \
    && update-ca-certificates

WORKDIR /workspace

# protobuf
RUN --mount=type=cache,target=/root/downloads \
    VERSION=3.19.4 \
    && URL=https://github.com/protocolbuffers/protobuf/releases/download/v${VERSION}/protobuf-cpp-${VERSION}.tar.gz \
    && FILE=protobuf-cpp-${VERSION}.tar.gz \
    && CACHE=/root/downloads/$FILE \
    && if [[ ! -e $CACHE ]] ; then \
           wget --quiet $URL -O $CACHE \
           && cd /root/downloads \
           && tar xf $CACHE \
           && cd protobuf-${VERSION} \
           && ./autogen.sh \
           && ./configure \
           && make -j$(($(nproc) - 1)) ; \
       fi \
    && cd /root/downloads/protobuf-${VERSION} \
    && make install

# pybind11
RUN --mount=type=cache,target=/root/downloads \
    VERSION=2.9.1 \
    && URL=https://github.com/pybind/pybind11/archive/refs/tags/v${VERSION}.tar.gz \
    && FILE=pybind11-${VERSION}.tar.gz \
    && CACHE=/root/downloads/$FILE \
    && if [[ ! -e $CACHE ]] ; then \
           wget --quiet $URL -O $CACHE \
           && cd /root/downloads \
           && tar xf $CACHE \
           && cd pybind11-${VERSION} \
           && mkdir build \
           && cd build \
           && cmake -DPYBIND11_TEST=OFF .. \
           && make -j$(($(nproc) - 1)) ; \
       fi \
    && cd /root/downloads/pybind11-${VERSION}/build \
    && make install

# json-c
RUN --mount=type=cache,target=/root/downloads \
    URL=https://github.com/json-c/json-c/archive/refs/tags/json-c-0.15-20200726.tar.gz \
    && FILE=json-c-0.15-20200726.tar.gz \
    && CACHE=/root/downloads/$FILE \
    && if [[ ! -e $CACHE ]] ; then \
           wget --quiet $URL -O $CACHE \
           && cd /root/downloads \
           && tar xf $CACHE \
           && cd json-c-json-c-0.15-20200726 \
           && mkdir build \
           && cd build \
           && cmake .. \
                  -DBUILD_SHARED_LIBS=ON \
                  -DBUILD_STATIC_LIBS=OFF \
                  -DBUILD_TESTING=OFF \
                  -DCMAKE_BUILD_TYPE=Release \
           && make -j$(($(nproc) - 1)) ; \
       fi \
    && cd /root/downloads/json-c-json-c-0.15-20200726/build \
    && make install

#ARG XRT_PACKAGE_URL=https://www.xilinx.com/bin/public/openDownload?filename=xrt_202120.2.12.427_20.04-amd64-xrt.deb
ARG XRT_PACKAGE_URL=https://github.com/Xilinx/video-sdk/raw/v2.0/release/U30_Ubuntu_20.04_v2.0/xrt_202110.2.11.691_20.04-amd64-xrt.deb
ARG XRT_APU_PACKAGE_URL

# Install XRT
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=cache,target=/root/downloads \
    if [[ -n "$XRT_PACKAGE_URL" ]] ; then \
        CACHE=/root/downloads/$(echo $XRT_PACKAGE_URL | sed 's/^.*[=\/]//') && \
        if [[ ! -e $CACHE ]] ; then \
            wget --quiet -O $CACHE "$XRT_PACKAGE_URL" ; \
        fi \
        && apt-get update \
        && apt-get install -y $CACHE ; \
    fi

# Install XRT_APU
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=cache,target=/root/downloads \
    if [[ -n "$XRT_APU_PACKAGE_URL" ]] ; then \
        CACHE=/root/downloads/$(echo $XRT_APU_PACKAGE_URL | sed 's/^.*[=\/]//') && \
        if [[ ! -e $CACHE ]] ; then \
            wget --quiet -O $CACHE "$XRT_APU_PACKAGE_URL" ; \
        fi \
        && apt-get update \
        && apt-get install -y $CACHE ; \
    fi

# Install XRM
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=xrm_202120.1.3.29_20.04-x86_64.deb \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && apt-get update \
    && apt-get install -y $CACHE

# Vitis AI
RUN --mount=type=cache,target=/root/downloads \
    if [[ ! -e /root/downloads/Vitis-AI ]] ; then \
        cd /root/downloads \
        && git clone --recursive --single-branch --branch v2.5 --depth 1 https://github.com/Xilinx/Vitis-AI.git ; \
    fi \
    && cp -a /root/downloads/Vitis-AI /workspace

RUN --mount=type=cache,target=/root/downloads \
    VITIS_ROOT=/root/downloads/Vitis-AI/src/Vitis-AI-Runtime/VART \
    && cd ${VITIS_ROOT}/unilog \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build \
    && cd ${VITIS_ROOT}/xir \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build --build-python \
    && cd ${VITIS_ROOT}/target_factory \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build \
    && cd ${VITIS_ROOT}/vart \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --cmake-options="-DBUILD_TEST=OFF" --build-python --build-dir ./build

RUN --mount=type=cache,target=/root/downloads \
    VITIS_ROOT=/root/downloads/Vitis-AI/src/Vitis-AI-Runtime/VART \
    && if [[ ! -e $VITIS_ROOT/rt-engine ]] ; then \
           git clone https://github.com/Xilinx/rt-engine.git ${VITIS_ROOT}/rt-engine ; \
       fi \
    && cd ${VITIS_ROOT}/rt-engine \
    && git checkout b32d379 \
    && ./cmake.sh --clean --build-dir=./build --type=release --cmake-options="-DXRM_DIR=/opt/xilinx/xrm/share/cmake" --cmake-options="-DBUILD_TESTS=OFF" --install-prefix /usr/local/

# Vitis AI Library
RUN --mount=type=cache,target=/root/downloads \
    cd /root/downloads/Vitis-AI/src/Vitis-AI-Library \
    && ./cmake.sh --clean --type=release --install-prefix /usr/local/ --build-dir ./build

# Install Vitis AI xclbins
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=DPUCAHX8H_xclbins_2_5_0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
           wget --quiet "$URL" -O $CACHE ; \
       fi \
    && tar xf $CACHE -C /

# Download BCC model
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=bcc_pt-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
    && tar xf $CACHE -C /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC

# Download YOLOv3
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=yolov3_voc_tf-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
    && tar xf $CACHE -C /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC

# Vitis AI Library images for test
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=vitis_ai_library_r2.5.0_images.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && tar xf $CACHE -C /workspace/Vitis-AI/examples/Vitis-AI-Library

# Gstreamer
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update \
    && apt-get install -y --no-install-recommends \
        gstreamer1.0-plugins-bad \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libgstrtspserver-1.0-dev \
        libswscale-dev \
        sudo

# U30 video-sdk 2.0
RUN --mount=type=cache,target=/root/downloads \
    if [[ ! -e /root/downloads/video-sdk ]] ; then \
        git clone -b v2.0 --depth 1 https://github.com/Xilinx/video-sdk.git /root/downloads/video-sdk \
        && cd /root/downloads/video-sdk/ \
        && git submodule update --init --recursive \
    ; fi \
    && cp -a /root/downloads/video-sdk /workspace

RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update \
    && cd /workspace/video-sdk/release/U30_Ubuntu_20.04_v2.0 \
    && ./install

# Dependencies for glvnd and X11.
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update \
    && apt-get install -y --no-install-recommends \
        libglvnd0 \
        libgl1 \
        libglx0 \
        libegl1 \
        libxext6 \
        libx11-6 \
        glmark2

RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update \
    && apt-get install -y --no-install-recommends \
        gdb \
        psmisc \
        less \
        vim-nox \
        ffmpeg \
        valgrind \
        gstreamer1.0-rtsp \
        gstreamer1.0-plugins-ugly \
        netcat

# rtsp-simple-server
RUN --mount=type=cache,target=/root/downloads \
    VER=v0.21.5 \
    && URL=https://github.com/aler9/rtsp-simple-server/releases/download/${VER}/rtsp-simple-server_${VER}_linux_amd64.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && tar xf $CACHE -C /usr/local/bin

# Patch
COPY files/yolov3_conf_threshold.patch /tmp
RUN patch /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC/yolov3_voc_tf/yolov3_voc_tf.prototxt < /tmp/yolov3_conf_threshold.patch

WORKDIR /workspace/demo
#CMD ["/workspace/demo/start.sh"]

FROM ubuntu:20.04

SHELL ["/bin/bash", "-c"]

ARG XRT_PACKAGE_URL
ARG XRT_APU_PACKAGE_URL
ARG XRT_PACKAGE=xrt_202220.2.14.418_20.04-amd64-xrt.deb
ARG XRT_APU_PACKAGE=xrt-apu_202220.2.14.418_petalinux_all.deb
ARG XRM_PACKAGE_URL=https://www.xilinx.com/bin/public/openDownload?filename=xrm_202220.1.5.212_20.04-x86_64.deb
ARG XRM_PACKAGE

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

RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=bind,source=packages,target=/packages \
    if [[ -n "$XRT_PACKAGE" ]] ; then \
        apt-get update \
        && apt-get install -y /packages/$XRT_PACKAGE ; \
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

RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=bind,source=packages,target=/packages \
    if [[ -n "$XRT_APU_PACKAGE" ]] ; then \
        apt-get update \
        && apt-get install -y /packages/$XRT_APU_PACKAGE ; \
    fi

# Install XRM
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=cache,target=/root/downloads \
    if [[ -n "$XRM_PACKAGE_URL" ]] ; then \
        CACHE=/root/downloads/$(echo $XRM_PACKAGE_URL | sed 's/^.*[=\/]//') && \
        if [[ ! -e $CACHE ]] ; then \
            wget --quiet -O $CACHE "$XRM_PACKAGE_URL" ; \
        fi \
        && apt-get update \
        && apt-get install -y $CACHE ; \
    fi

RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=bind,source=packages,target=/packages \
    if [[ -n "$XRM_PACKAGE" ]] ; then \
        apt-get update \
        && apt-get install -y /packages/$XRM_PACKAGE ; \
    fi

# Vitis AI
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=vairuntime-3.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && tar xf $CACHE -C /tmp \
    && apt-get update \
    && apt-get install /tmp/vairuntime/*.deb \
    && rm -rf /tmp/vairuntime \
    && ldconfig \
    && cd /usr/lib && mv libvart-dpu-runner.so libvart-dpu-runner.so_org && ln -s librt-engine.so libvart-dpu-runner.so

# # Download BCC model
# RUN --mount=type=cache,target=/root/downloads \
#     URL=https://www.xilinx.com/bin/public/openDownload?filename=bcc_pt-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
#     && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
#     && if [[ ! -e $CACHE ]] ; then \
#         wget --quiet -O $CACHE "$URL" ; \
#     fi \
#     && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
#     && tar xf $CACHE -C /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC
# 
# # Download YOLOv3
# RUN --mount=type=cache,target=/root/downloads \
#     URL=https://www.xilinx.com/bin/public/openDownload?filename=yolov3_voc_tf-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
#     && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
#     && if [[ ! -e $CACHE ]] ; then \
#         wget --quiet -O $CACHE "$URL" ; \
#     fi \
#     && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
#     && tar xf $CACHE -C /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC

# # Vitis AI Library images for test
# RUN --mount=type=cache,target=/root/downloads \
#     URL=https://www.xilinx.com/bin/public/openDownload?filename=vitis_ai_library_r3.0.0_images.tar.gz \
#     && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
#     && if [[ ! -e $CACHE ]] ; then \
#         wget --quiet -O $CACHE "$URL" ; \
#     fi \
#     && tar xf $CACHE -C /workspace/Vitis-AI/examples/vai_library

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
        netcat \
        iputils-ping

# rtsp-simple-server
RUN --mount=type=cache,target=/root/downloads \
    VER=v0.21.5 \
    && URL=https://github.com/aler9/rtsp-simple-server/releases/download/${VER}/rtsp-simple-server_${VER}_linux_amd64.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && tar xf $CACHE -C /usr/local/bin

# Install V70 VVAS packages
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    --mount=type=bind,source=packages,target=/packages \
    apt-get update \
    && apt-get install -y \
        /packages/v70-xclbin_1.0.0_20.04-amd64.deb \
        /packages/vvas_3.0.5_20.04-amd64.deb \
        /packages/vvas-core_1.0.4_20.04-amd64.deb \
        /packages/vvas-gstreamer_1.18.5.0_20.04-amd64.deb \
        /packages/vvas-v70-examples_1.0.0_20.04-amd64.deb

# # Patch
# COPY files/yolov3_conf_threshold.patch /tmp
# RUN patch /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC/yolov3_voc_tf/yolov3_voc_tf.prototxt < /tmp/yolov3_conf_threshold.patch

WORKDIR /workspace/demo

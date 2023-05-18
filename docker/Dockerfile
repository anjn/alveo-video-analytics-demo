ARG CARD=${CARD:-v70}
ARG VAI_VER=${VAI_VER:-30}

## base
## runtime
## vai_base
##     vai_25
##     vai_30
## vai
##     demo_v70
##     demo_u30_u5x
## demo

##===========================================================================
## base
##===========================================================================

FROM ubuntu:20.04 as base

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# Timezone
ENV TZ=Asia/Tokyo
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# # Clean cache
# RUN --mount=type=cache,target=/var/lib/apt/lists \
#     --mount=type=cache,target=/var/cache/apt/archives \
#     rm -rf /var/lib/apt/lists/* \
#     && apt-get clean
# 
# # Clean cache
# RUN --mount=type=cache,target=/root/downloads \
#     rm -rf /root/downloads/*

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
        ffmpeg \
        gdb \
        git \
        iputils-ping \
        less \
        libeigen3-dev \
        libgoogle-glog-dev \
        libgtest-dev \
        libopencv-dev \
        libssl-dev \
        libtool \
        libunwind-dev \
        netcat \
        net-tools \
        pkg-config \
        psmisc \
        python3-dev \
        python3-pip \
        python3-setuptools \
        python3-wheel \
        sudo \
        unzip \
        valgrind \
        vim-nox \
        wget \
    && update-ca-certificates

# Gstreamer
RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update \
    && apt-get install -y --no-install-recommends \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-rtsp \
        gstreamer1.0-plugins-ugly \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libgstrtspserver-1.0-dev \
        libswscale-dev

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

##===========================================================================
## runtime
##===========================================================================

FROM base as runtime

ARG XRT_PACKAGE_URL
ARG XRT_PACKAGE
ARG XRT_APU_PACKAGE_URL
ARG XRT_APU_PACKAGE
ARG XRM_PACKAGE_URL
ARG XRM_PACKAGE

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

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

##===========================================================================
## vai_base
##===========================================================================

FROM runtime as vai_base

ARG PROTOBUF_VER=${PROTOBUF_VER:-3.6.1}

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# protobuf
RUN --mount=type=cache,target=/root/downloads \
    VERSION=$PROTOBUF_VER \
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

RUN ldconfig

##===========================================================================
## vai_25
##===========================================================================

FROM vai_base as vai_25

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# Vitis AI 2.5
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

##===========================================================================
## vai_30
##===========================================================================

FROM vai_base as vai_30

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# Vitis AI 3.0
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

##===========================================================================
## vai
##===========================================================================

FROM vai_${VAI_VER} as vai

##===========================================================================
## demo_v70
##===========================================================================

FROM vai as demo_v70

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

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

##===========================================================================
## demo_u30_u5x
##===========================================================================

FROM vai as demo_u30_u5x

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

RUN mkdir -p /workspace

## # U30 video-sdk 2.0
## RUN --mount=type=cache,target=/root/downloads \
##     if [[ ! -e /root/downloads/video-sdk ]] ; then \
##         git clone -b release_2.0 --depth 1 https://github.com/Xilinx/video-sdk.git /root/downloads/video-sdk \
##         && cd /root/downloads/video-sdk/ \
##         && git submodule update --init --recursive \
##     ; fi \
##     && cp -a /root/downloads/video-sdk /workspace
## 
## RUN --mount=type=cache,target=/var/lib/apt/lists \
##     --mount=type=cache,target=/var/cache/apt/archives \
##     apt-get update \
##     && cd /workspace/video-sdk/release/U30_Ubuntu_20.04_v2.0 \
##     && ./install

## # U30 video-sdk 2.0.1
## RUN --mount=type=cache,target=/root/downloads \
##     if [[ ! -e /root/downloads/video-sdk ]] ; then \
##         git clone -b v2.0 --depth 1 https://github.com/Xilinx/video-sdk.git /root/downloads/video-sdk \
##         && cd /root/downloads/video-sdk/ \
##         && git submodule update --init --recursive \
##     ; fi \
##     && cp -a /root/downloads/video-sdk /workspace
## 
## RUN --mount=type=cache,target=/var/lib/apt/lists \
##     --mount=type=cache,target=/var/cache/apt/archives \
##     apt-get update \
##     && cd /workspace/video-sdk/release/U30_Ubuntu_20.04_v2.0.1 \
##     && ./install

# U30 Video SDK 3.0
RUN echo "deb [trusted=yes] https://packages.xilinx.com/artifactory/debian-packages $(lsb_release -c | awk '{print $2}') main" > /etc/apt/sources.list.d/xilinx.list

RUN --mount=type=cache,target=/var/lib/apt/lists \
    --mount=type=cache,target=/var/cache/apt/archives \
    apt-get update \
    && apt-get install -y --no-install-recommends xrt=2.11.722 \
    && apt-mark hold xrt \
    && apt-get install -y --no-install-recommends \
        xilinx-alveo-u30-core \
        xilinx-alveo-u30-ffmpeg \
        xilinx-alveo-u30-gstreamer \
        xilinx-alveo-u30-examples \
    && cp /opt/xilinx/xcdr/xclbins/transcode.xclbin /opt/xilinx/xcdr/xclbins/transcode_lite.xclbin \
    && cp /opt/xilinx/xcdr/xclbins/on_prem/transcode.xclbin /opt/xilinx/xcdr/xclbins/transcode.xclbin

# Download BCC model
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=bcc_pt-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
    && tar xf $CACHE -C /usr/share/vitis_ai_library/models

# Download YOLOv3
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=yolov3_voc_tf-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
    && tar xf $CACHE -C /usr/share/vitis_ai_library/models

# Download Car models
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=chen_color_resnet18_pt-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
    && tar xf $CACHE -C /usr/share/vitis_ai_library/models

RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=vehicle_make_resnet18_pt-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
    && tar xf $CACHE -C /usr/share/vitis_ai_library/models

RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=vehicle_type_resnet18_pt-u55c-u50lv-DPUCAHX8H-DWC-r2.5.0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && mkdir -p /usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC \
    && tar xf $CACHE -C /usr/share/vitis_ai_library/models

# Patch
COPY files/yolov3_conf_threshold.patch /tmp
RUN patch /usr/share/vitis_ai_library/models/yolov3_voc_tf/yolov3_voc_tf.prototxt < /tmp/yolov3_conf_threshold.patch

# Install Vitis AI xclbins
RUN --mount=type=cache,target=/root/downloads \
    URL=https://www.xilinx.com/bin/public/openDownload?filename=DPUCAHX8H_xclbins_2_5_0.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
           wget --quiet "$URL" -O $CACHE ; \
       fi \
    && tar xf $CACHE -C /

##===========================================================================
## demo
##===========================================================================

FROM demo_${CARD} as demo

ARG CARD_ML=${CARD_ML:-v70}
ARG CARD_VIDEO=${CARD_VIDEO:-v70}

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# rtsp-simple-server
RUN --mount=type=cache,target=/root/downloads \
    VER=v0.21.5 \
    && URL=https://github.com/aler9/rtsp-simple-server/releases/download/${VER}/rtsp-simple-server_${VER}_linux_amd64.tar.gz \
    && CACHE=/root/downloads/$(echo $URL | sed 's/^.*[=\/]//') \
    && if [[ ! -e $CACHE ]] ; then \
        wget --quiet -O $CACHE "$URL" ; \
    fi \
    && tar xf $CACHE -C /usr/local/bin

COPY files/entrypoint.sh /

ENV CARD_ML=${CARD_ML}
ENV CARD_VIDEO=${CARD_VIDEO}
WORKDIR /workspace/demo

ENTRYPOINT ["/entrypoint.sh"]


# Video+ML demo with Alveo U30 and U50LV

This demo shows
- Simultaneous 32-ch 1080p15 low latency video decoding using U30
- Real time crowd counting AI inference using U50LV

with low CPU usage.

## System diagram
![](images/system.png)

## Demo
![](images/demo.jpg)

## Prerequisites

- Host server that supports PCIe bifurcation x4x4
- Alveo U30
- Alveo U50LV or U55C
- Ubuntu 20.04

## Installation

Please see [INSTALL.md](INSTALL.md) for installation.

## Build Docker image

```bash
$ ./docker/build.sh
```

## Prepare videos

Use `videos/transcode.sh` to transcode video clips into a suitable foramt for RTSP streaming. To use this script, `ffmpeg` needs to be installed.

```bash
./videos/transcode.sh VIDEO_CLIP
```

## Edit config.toml

### Add cameras

```toml
[video]
cameras = [
    # YOLOv3 person detection
    { location = "rtsp://localhost:8554/test00", model = "yolov3", labels = [ 14 ] },
    # YOLOv3 bicyle/bus/car/motorbike detection
    { location = "rtsp://localhost:8554/test01", model = "yolov3", labels = [ 1, 5, 6, 13 ] },
    # Crowd counting
    { location = "rtsp://localhost:8554/test02", model = "bcc" },
```

## Run demo

```bash
$ ./start.sh
```

## Stop demo

```bash
$ ./stop.sh
```

## Third party software
- https://github.com/zeromq/libzmq
- https://github.com/zeromq/cppzmq
- https://github.com/msgpack/msgpack-c
- https://github.com/ToruNiina/toml11
- https://github.com/gabime/spdlog
- https://github.com/anjn/arg

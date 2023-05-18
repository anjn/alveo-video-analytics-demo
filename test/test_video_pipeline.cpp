#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"
#include "video/gst_app_bgr_utils.hpp"

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    arg_begin("", 0, 0);
    arg_s(location, "/opt/xilinx/examples/Videos/Cars_1900.264", "Video file.");
    arg_s(rtsp_server_location, "", "");
    arg_i(dev_idx, 0, "Device index.");
    arg_i(width, 960, "Width");
    arg_i(height, 540, "Height");
    arg_end;

    // Input
    multifilesrc src0 { location };
    vvas_dec dec { dev_idx };
    vvas_scaler scaler { dev_idx, width, height };
    videoconvert conv0;
    rawvideomedia raw0;
    raw0.format = "BGR";
    appsink sink0;

    auto pipeline0 = build_pipeline_and_play(src0, dec, scaler, conv0, raw0, sink0);

    std::vector<std::tuple<GstAppSink**, cv::Size>> appsinks;
    appsinks.emplace_back(&sink0.sink, cv::Size(width, height));

    app2queue_bgr::queue_ptr_t queue = std::make_shared<app2queue_bgr::queue_t>();
    app2queue_bgr a2q { appsinks, queue };
    a2q.start();

    // Output
    appsrc src1;
    videoconvert conv1;
    rawvideomedia raw1;
    raw1.format = "NV12";
    hw_enc enc { dev_idx };
    rtspclientsink sink1(rtsp_server_location);

    auto pipeline1 = build_pipeline_and_play(src1, conv1, raw1, enc, sink1);

    queue2app_bgr q2a { queue, src1.src, width, height };
    q2a.start();

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}


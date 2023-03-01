#include <vector>

#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"
#include "video/gst_app_utils.hpp"
#include "video/rtsp_server.hpp"
#include "video/hw_config_u30.hpp"

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    arg_begin("", 0, 0);
    arg_s(location, "rtsp://localhost:8554/test00", "RTSP stream URL.");
    arg_i(dev_idx, 0, "Device index.");
    arg_i(width, 960, "Width");
    arg_i(height, 540, "Height");
    arg_i(port, 8555, "Port");
    arg_end;

    // Create rtsp server pipeline
    rtsp_server server { "test", port };
    server.start();

    // Create rtsp receiver pipeline
    rtsp_receiver receiver { location };
    vvas_dec<hw_config_u30> dec { dev_idx };
    vvas_scaler<hw_config_u30> scaler { dev_idx, width, height };
    appsink sink {};

    auto pipeline0 = build_pipeline(receiver, dec, scaler, sink);
    gst_element_set_state(pipeline0, GST_STATE_PLAYING);

    appsrc src1 {};
    intervideosink sink1 { "test" };

    auto pipeline1 = build_pipeline(src1, sink1);
    gst_element_set_state(pipeline1, GST_STATE_PLAYING);

    // Connect app sink and source by queue
    auto queue = create_app_queue();

    app2queue a2q { sink.sink, queue, width, height };
    a2q.start();
    queue2app q2a { queue, src1.src };
    q2a.start();

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}


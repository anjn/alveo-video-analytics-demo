#include <vector>

#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"
#include "video/rtsp_server.hpp"

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    arg_begin("", 0, 0);
    arg_i(width, 960, "Width");
    arg_i(height, 540, "Height");
    arg_i(port, 8555, "Port");
    arg_i(device_index, 0, "Device index");
    arg_end;

    rtsp_server server { "test", port, device_index };
    server.start();

    videotestsrc src { width, height, 0, "NV12" };
    intervideosink sink { "test" };

    auto pipeline = build_pipeline(src, sink);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}

#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    arg_begin("", 0, 0);
    arg_s(location, "rtsp://localhost:8554/demo", "RTSP");
    arg_i(width, 960, "Width");
    arg_i(height, 540, "Height");
    arg_end;

    videotestsrc src(width, height);
    x264enc enc;
    rtspclientsink sink(location);

    auto pipeline = build_pipeline(src, enc, sink);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}

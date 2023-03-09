#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    arg_begin("", 0, 0);
    arg_i(width, 960, "Width");
    arg_i(height, 540, "Height");
    arg_end;

    videotestsrc src;

    rawvideomedia raw;
    raw.width = width;
    raw.height = height;
    raw.framerate_n = 15;
    raw.format = "I420";

    autovideosink sink;

    auto pipeline = build_pipeline(src, sink);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}

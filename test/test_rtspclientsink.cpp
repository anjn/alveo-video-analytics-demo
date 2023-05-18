#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    arg_begin("", 0, 0);
    arg_s(location, "rtsp://localhost:8554/test", "RTSP");
    arg_i(width, 960, "Width");
    arg_i(height, 540, "Height");
    arg_i(device_index, 0, "HW device index");
    arg_end;

    videotestsrc src;

    rawvideomedia raw;
    raw.width = width;
    raw.height = height;
    raw.framerate_n = 15;
    raw.format = "NV12";

    hw_enc enc {};

    rtspclientsink sink(location);

    build_pipeline_and_play(src, raw, enc, sink);

//    if (hw_enc)
//    {
//        raw.format = "NV12";
//        vvas_enc enc(device_index);
//    }
//    else
//    {
//        x264enc enc;
//        build_pipeline_and_play(src, raw, enc, sink);
//    }

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}

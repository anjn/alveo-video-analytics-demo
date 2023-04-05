#include <vector>

#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"
#include "video/hw_config_v70.hpp"

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    arg_begin("", 0, 0);
    arg_s(location, "/opt/xilinx/examples/Videos/Cars_1900.264", "Video file.");
    arg_i(dev_idx, 0, "Device index.");
    arg_i(width, 960, "Width");
    arg_i(height, 540, "Height");
    arg_end;

    multifilesrc src { location };
    vvas_dec<hw_config_v70> dec { dev_idx };
    vvas_scaler<hw_config_v70> scaler { dev_idx, width, height };
    fpsdisplaysink sink {};

    auto pipeline = build_pipeline(src, dec, scaler, sink);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}

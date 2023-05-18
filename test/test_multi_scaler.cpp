#include <vector>

#include <arg/arg.h>

#include "video/gst_pipeline_utils.hpp"
#include "video/hw_config_u30.hpp"
#include "video/hw_config_v70.hpp"

struct geometry {
  int w, h;
};

namespace arg {
namespace helper {

template<>
struct option<geometry> {
  static geometry cast(const std::string& value) {
    // find 'x'
    size_t pos = value.find('x', 0);
    if (pos == std::string::npos || pos == value.size()-1)
      throw std::invalid_argument("invalid input");

    geometry v;
    v.w = option<int>::cast(value.substr(0,pos));
    v.h = option<int>::cast(value.substr(pos+1));
    return v;
  }
  static bool require_value() { return true; }
  static std::string name() { return "<width>x<height>"; }
};

}
}

struct multi_scaler
{
    std::vector<geometry> sizes;
    //vvas_scaler scaler;
    sw_multi_scaler scaler;

    GstElement* pipeline;

    multi_scaler(std::vector<geometry> sizes_) :
        sizes(sizes_)
        //scaler(0)
    {
        //scaler.multi_scaler = true;
    }

    std::string get_description()
    {
        std::string desc = scaler.get_description();

        for (int i = 0; i < sizes.size(); i++)
        {
            //desc += " " + scaler.scaler_name + ".src_" + std::to_string(i);
            desc += " " + scaler.scaler_name + ".";
            desc += " ! queue";
            desc += " ! videoscale";
            desc += " ! videoconvert";
            desc += " ! video/x-raw, width=" + std::to_string(sizes[i].w) + ", height=" + std::to_string(sizes[i].h) + ", format=BGR";
            desc += " ! queue";
            desc += " ! videoconvert";
            desc += " ! autovideosink";
        }

        return desc;
    }

    void set_params(GstElement* pipeline)
    {
        ::set_params(pipeline, scaler);
    }
};

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

    std::vector<geometry> sizes;

    arg_begin("", 0, 0);
    arg_s(location, "/opt/xilinx/examples/Videos/Cars_1900.264", "Video file.");
    arg_i(dev_idx, 0, "Device index.");
    arg_g(sizes, 960x540, "Scaler output sizes");
    arg_end;

    multifilesrc src { location };
    vvas_dec dec { dev_idx };
    multi_scaler scaler { sizes };

    auto pipeline = build_pipeline_and_play(src, dec, scaler);

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}


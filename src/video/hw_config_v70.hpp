#pragma once
#include <cassert>
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
extern "C" {
#include <gst/gst.h>
}
#pragma GCC diagnostic pop

struct hw_config_v70
{
    inline static const std::string decoder_element_name = "vvas_xvideodec";
    inline static const std::string xclbin = "/opt/xilinx/xclbin/v70.xclbin";
    inline static const int num_devices = 1;
    inline static const int num_decoder = 16;
    inline static const int num_scaler = 2;

    static void set_decoder_params(
        GstElement* elm,
        int dev_idx = 0
    ) {
        int decoder_idx = dev_idx % num_decoder; // soft kernel index
        dev_idx = 0;

        assert(elm);
        assert(0 <= dev_idx && dev_idx < num_devices);
        assert(0 <= decoder_idx && decoder_idx < num_decoder);

        const int vdu_idx = decoder_idx / 8;
        const std::string decoder_kernel = "kernel_vdu_decoder:{kernel_vdu_decoder_" + std::to_string(decoder_idx) + "}";

        g_object_set(G_OBJECT(elm),
                     "xclbin-location", xclbin.c_str(),
                     "kernel-name", decoder_kernel.c_str(),
                     "dev-idx", dev_idx,
                     "instance-id", vdu_idx,
                     //"low-latency", true,
                     //"splitbuff-mode", false,
                     "avoid-output-copy", true,
                     "avoid-dynamic-alloc", true,
                     nullptr);
    }

    static void set_scaler_params(
        GstElement* elm,
        int dev_idx = 0
    ) {
        int scaler_idx = (dev_idx / (num_decoder / num_scaler)) % num_scaler;
        dev_idx = 0;

        static_assert(num_decoder % num_scaler == 0);
        assert(elm);
        assert(0 <= dev_idx && dev_idx < num_devices);
        assert(0 <= scaler_idx && scaler_idx < num_scaler);

        const std::string scaler_kernel = "image_processing:{image_processing_" + std::to_string(scaler_idx + 1) + "}";

        g_object_set(G_OBJECT(elm),
                     "xclbin-location", xclbin.c_str(),
                     "kernel-name", scaler_kernel.c_str(),
                     "dev-idx", dev_idx,
                     "ppc", 4,
                     "scale-mode", 2, // polyphase
                     "coef-load-type", 1, // auto
                     "avoid-output-copy", true,
                     nullptr);
    }
};


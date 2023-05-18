#pragma once
#include <cassert>
#include <string>

#include "video/hw_config_base.hpp"

struct hw_config_v70 : public hw_config_base
{
    inline static const std::string decoder_element_name = "vvas_xvideodec";
    inline static const std::string xclbin = "/opt/xilinx/xclbin/v70.xclbin";
    inline static const int num_decoder = 16;
    inline static const int num_scaler = 2;

    int num_devices = 2;

    std::string get_decoder_element_name() override { return "vvas_xvideodec"; };
    std::string get_scaler_element_name() override { return "vvas_xabrscaler"; };

    void set_num_devices(int num) override { num_devices = num; }
    int get_num_devices() override { return num_devices; };

    void set_decoder_params(
        GstElement* elm,
        int dev_idx
    ) override {
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
                     "low-latency", true,
                     "splitbuff-mode", false,
                     "avoid-output-copy", true,
                     "avoid-dynamic-alloc", true,
                     nullptr);
    }

    void set_scaler_params(
        GstElement* elm,
        int dev_idx
    ) override {
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


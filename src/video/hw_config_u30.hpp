#pragma once
#include <cassert>
#include <string>

#include "video/hw_config_base.hpp"

struct hw_config_u30 : public hw_config_base
{
    int num_devices = 2;

    std::string get_decoder_element_name() override { return "vvas_xvcudec"; };
    std::string get_scaler_element_name() override { return "vvas_xabrscaler"; };
    std::string get_encoder_element_name() override { return "vvas_xvcuenc"; };

    bool support_scaler_bgr_output() override { return false; }

    void set_num_devices(int num) override { num_devices = num; }
    int get_num_devices() override { return num_devices; };

    void set_decoder_params(
        GstElement* elm,
        int dev_idx
    ) override {
        dev_idx %= num_devices;

        assert(elm);
        assert(0 <= dev_idx && dev_idx < num_devices);

        g_object_set(G_OBJECT(elm),
                     "dev-idx", dev_idx,
                     "low-latency", true,
                     "splitbuff-mode", false,
                     "avoid-output-copy", true,
                     nullptr);
    }

    void set_encoder_params(
        GstElement* elm,
        int dev_idx,
        int bitrate
    ) override {
        dev_idx %= num_devices;

        assert(elm);
        assert(0 <= dev_idx && dev_idx < num_devices);

        g_object_set(G_OBJECT(elm),
                     "dev-idx", dev_idx,
                     "target-bitrate", bitrate,
                     "max-bitrate", bitrate,
                     "b-frames", 0,
                     "control-rate", 3,
                     "scaling-list", 0,
                     "rc-mode", false,
                     nullptr);
    }

    void set_scaler_params(
        GstElement* elm,
        int dev_idx
    ) override {
        dev_idx %= num_devices;

        assert(elm);
        assert(0 <= dev_idx && dev_idx < num_devices);

        g_object_set(G_OBJECT(elm),
                     "dev-idx", dev_idx,
                     "ppc", 4,
                     "scale-mode", 1,
                     "num-taps", 12,
                     nullptr);
    }
};


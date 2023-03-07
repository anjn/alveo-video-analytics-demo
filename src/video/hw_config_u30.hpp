#pragma once
#include <cassert>
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
extern "C" {
#include <gst/gst.h>
}
#pragma GCC diagnostic pop

struct hw_config_u30
{
    static inline int max_devices = 2;

    static void set_decoder_params(
        GstElement* elm,
        int dev_idx = 0
    ) {
        dev_idx %= max_devices;

        assert(elm);
        assert(0 <= dev_idx && dev_idx < max_devices);

        g_object_set(G_OBJECT(elm),
                     "dev-idx", dev_idx,
                     "low-latency", true,
                     "splitbuff-mode", false,
                     "avoid-output-copy", true,
                     nullptr);
    }

    static void set_scaler_params(
        GstElement* elm,
        int dev_idx = 0
    ) {
        dev_idx %= max_devices;

        assert(elm);
        assert(0 <= dev_idx && dev_idx < max_devices);

        g_object_set(G_OBJECT(elm),
                     "dev-idx", dev_idx,
                     "ppc", 4,
                     "scale-mode", 2,
                     nullptr);
    }
};


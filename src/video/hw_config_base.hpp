#pragma once
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//extern "C" {
#include <gst/gst.h>
//}
#pragma GCC diagnostic pop

struct hw_config_base
{
    virtual std::string get_decoder_element_name() { return ""; };
    virtual std::string get_scaler_element_name() { return ""; };
    virtual std::string get_encoder_element_name() { return ""; };

    virtual bool support_scaler_bgr_output() { return true; }

    virtual void set_num_devices(int num) = 0;
    virtual int get_num_devices() = 0;

    virtual void set_decoder_params(GstElement* elm, int dev_idx) {};
    virtual void set_scaler_params(GstElement* elm, int dev_idx) {};
    virtual void set_encoder_params(GstElement* elm, int dev_idx, int bitrate) {};
};


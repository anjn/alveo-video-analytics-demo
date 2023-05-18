#pragma once
#include <iostream>
#include <atomic>
#include <cassert>
#include <gst/gstelement.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
extern "C" {
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
}
#pragma GCC diagnostic pop

#include "video/hw_config.hpp"
#include "utils/queue_mt.hpp"

std::string get_new_name(const std::string& base)
{
    static std::atomic<int> id;
    return base + std::to_string(id++);
}

struct gst_element_base
{
    virtual std::string get_description() = 0;
    virtual void set_params(GstElement* pipeline) = 0;
};

struct rtsp_receiver
{
    const std::string location;

    const std::string src_name;

    rtsp_receiver(
        const std::string& location_
    ) : 
        location(location_),
        src_name(get_new_name("src"))
    {}

    std::string get_description()
    {
        return
            " rtspsrc name=" + src_name +
            " ! queue"
            " ! rtph264depay"
            " ! h264parse"
            ;
    }

    void set_params(GstElement* pipeline)
    {
        auto src = gst_bin_get_by_name(GST_BIN(pipeline), src_name.c_str());
        assert(src);
        g_object_set(G_OBJECT(src), "location", location.c_str(), nullptr);
        g_object_set(G_OBJECT(src), "latency", 200, nullptr);
        g_object_set(G_OBJECT(src), "do-rtcp", true, nullptr);
        g_object_set(G_OBJECT(src), "ntp-sync", false, nullptr);
        g_object_set(G_OBJECT(src), "drop-on-latency", true, nullptr);
        g_object_set(G_OBJECT(src), "protocols", "tcp", nullptr);
        //g_object_set(G_OBJECT(src), "is-live", true, nullptr);
        gst_object_unref(src);
    }
};

struct rtspclientsink
{
    const std::string location;
    const std::string sink_name;

    rtspclientsink(
        const std::string& location_
    ) : 
        location(location_),
        sink_name(get_new_name("sink"))
    {}

    std::string get_description()
    {
        return
            " rtspclientsink name=" + sink_name
            ;
    }

    void set_params(GstElement* pipeline)
    {
        auto sink = gst_bin_get_by_name(GST_BIN(pipeline), sink_name.c_str());
        assert(sink);
        g_object_set(G_OBJECT(sink),
                     "location", location.c_str(),
                     "latency", 500,
                     "protocols", "tcp",
                     nullptr);
        gst_object_unref(sink);
    }
};

struct multifilesrc
{
    const std::string location;

    const std::string src_name;

    multifilesrc(
        const std::string& location_
    ) : 
        location(location_),
        src_name(get_new_name("src"))
    {}

    std::string get_description()
    {
        return
            " multifilesrc name=" + src_name +
            " ! h264parse"
            ;
    }

    void set_params(GstElement* pipeline)
    {
        auto src = gst_bin_get_by_name(GST_BIN(pipeline), src_name.c_str());
        assert(src);
        g_object_set(G_OBJECT(src), "location", location.c_str(), nullptr);
        gst_object_unref(src);
    }
};

struct videotestsrc
{
    const int pattern;

    videotestsrc(int pattern_ = 0) : 
        pattern(pattern_)
    {}

    std::string get_description()
    {
        return
            " videotestsrc pattern=" + std::to_string(pattern)
            ;
    }

    void set_params(GstElement* pipeline) {}
};

struct videoconvert
{
    std::string get_description() { return "videoconvert"; }
    void set_params(GstElement* pipeline) {}
};

struct rawvideomedia
{
    int width;
    int height;
    int framerate_n;
    int framerate_d;
    std::string format;

    rawvideomedia() : 
        width(0),
        height(0),
        framerate_n(0),
        framerate_d(0),
        format("")
    {}

    std::string get_description()
    {
        std::vector<std::string> params;
        if (width > 0) params.emplace_back("width=" + std::to_string(width));
        if (height > 0) params.emplace_back("height=" + std::to_string(height));
        if (framerate_n > 0) {
            int d = framerate_d > 0 ? framerate_d : 1;
            params.emplace_back("framerate=" + std::to_string(framerate_n) + "/" + std::to_string(d));
        }
        if (format.size() > 0) params.emplace_back("format=" + format);

        std::string desc = "video/x-raw";

        for (auto p : params) {
            desc += ", " + p;
        }

        return desc;
    }

    void set_params(GstElement* pipeline) {}
};

struct appsrc
{
    GstAppSrc* src;

    const std::string src_name;

    appsrc() : 
        src(nullptr),
        src_name(get_new_name("src"))
    {}

    ~appsrc()
    {
        gst_object_unref(src);
    }

    std::string get_description()
    {
        return
            " appsrc name=" + src_name
            ;
    }

    void set_params(GstElement* pipeline)
    {
        src = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline), src_name.c_str()));
        assert(src);
        g_object_set(G_OBJECT(src),
                     "is-live", true,
                     "emit-signals", true,
                     "format", GST_FORMAT_TIME,
                     nullptr);
    }
};

struct vvas_dec
{
    const int dev_idx;

    const std::string dec_name;

    vvas_dec(int dev_idx_) :
        dev_idx(dev_idx_),
        dec_name(get_new_name("dec"))
    {}

    std::string get_description()
    {
        return
            " " + hw_config::get_instance()->get_decoder_element_name() + " name=" + dec_name
            ;
    }

    void set_params(GstElement* pipeline)
    {
        auto dec = gst_bin_get_by_name(GST_BIN(pipeline), dec_name.c_str());
        assert(dec);
        hw_config::get_instance()->set_decoder_params(dec, dev_idx);
        gst_object_unref(dec);
    }
};

struct vvas_scaler
{
    const int dev_idx;
    const int width;
    const int height;
    const std::string format;
    bool multi_scaler;

    const std::string scaler_name;

    vvas_scaler(
        int dev_idx_,
        int width_ = 0,
        int height_ = 0,
        const std::string& format_ = "NV12",
        bool multi_scaler_ = false
    ) : 
        dev_idx(dev_idx_),
        width(width_),
        height(height_),
        format(format_),
        multi_scaler(multi_scaler_),
        scaler_name(get_new_name("scaler"))
    {}

    std::string get_description()
    {
        std::string desc =
            " " + hw_config::get_instance()->get_scaler_element_name() + " name=" + scaler_name;

        if (!multi_scaler) {
            assert(width > 0);
            assert(height > 0);
            assert(format != "");

            desc += " ! video/x-raw, width=" + std::to_string(width) +
                    ", height=" + std::to_string(height) + ", format=" + format;
        }

        return desc;
    }

    void set_params(GstElement* pipeline)
    {
        auto scaler = gst_bin_get_by_name(GST_BIN(pipeline), scaler_name.c_str());
        assert(scaler);
        hw_config::get_instance()->set_scaler_params(scaler, dev_idx);
        gst_object_unref(scaler);
    }
};

struct sw_multi_scaler
{
    const std::string scaler_name;

    sw_multi_scaler() : 
        scaler_name(get_new_name("scaler"))
    {}

    std::string get_description()
    {
        std::string desc;
        desc += "   tee name=" + scaler_name;
        desc += " ! queue";
        desc += " ! fakesink";
        return desc;
    }

    void set_params(GstElement* pipeline) {}

    std::string get_source_pad(int index)
    {
        return scaler_name + ".";
    }
};

struct enc_base : gst_element_base
{
    virtual void set_bitrate(int value) = 0;
};

struct x264enc : enc_base
{
    const std::string name;

    int bitrate;

    x264enc() :
        name(get_new_name("enc")),
        bitrate(16000)
    {}

    void set_bitrate(int value) override
    {
        bitrate = value;
    }

    std::string get_description() override
    {
        return " x264enc name=" + name + " speed-preset=ultrafast tune=zerolatency sliced-threads=true key-int-max=15 ref=1 bframes=0 cabac=0";
    }

    void set_params(GstElement* pipeline) override
    {
        auto enc = gst_bin_get_by_name(GST_BIN(pipeline), name.c_str());
        assert(enc);
        g_object_set(G_OBJECT(enc),
                     "bitrate", bitrate,
                     nullptr);
        gst_object_unref(enc);
    }
};

struct vvas_enc : enc_base
{
    const int dev_idx;
    const std::string enc_name;

    int bitrate;

    vvas_enc(int dev_idx_ = 0) :
        dev_idx(dev_idx_),
        enc_name(get_new_name("enc")),
        bitrate(16000)
    { }

    void set_bitrate(int value) override
    {
        bitrate = value;
    }

    std::string get_description() override
    {
        return
            " " + hw_config::get_instance()->get_encoder_element_name() + " name=" + enc_name +
            " ! video/x-h264"
            ;
    }

    void set_params(GstElement* pipeline) override
    {
        auto enc = gst_bin_get_by_name(GST_BIN(pipeline), enc_name.c_str());
        assert(enc);
        hw_config::get_instance()->set_encoder_params(enc, dev_idx, bitrate);
        gst_object_unref(enc);
    }
};

struct hw_enc : enc_base
{
    std::shared_ptr<enc_base> elm;

    hw_enc(int dev_idx = 0)
    {
        bool enable = false;
        if (hw_config::get_instance()->get_encoder_element_name() != "") enable = true;
        if (const char* e = std::getenv("DISABLE_HW_ENC"))
            if (std::string(e) == "1") enable = false;

        if (enable) {
            std::cout << "HW enc enabled" << std::endl;
            elm = std::make_shared<vvas_enc>(dev_idx);
        } else {
            std::cout << "HW enc disabled" << std::endl;
            elm = std::make_shared<x264enc>();
        }
    }

    void set_bitrate(int value) override
    {
        elm->set_bitrate(value);
    }

    std::string get_description() override
    {
        return elm->get_description();
    }

    void set_params(GstElement* pipeline) override
    {
        elm->set_params(pipeline);
    }
};

struct fpsdisplaysink
{
    std::string get_description()
    {
        return
            " videoconvert"
            "! fpsdisplaysink video-sink=\"ximagesink async=false sync=false\" sync=false"
            ;
    }

    void set_params(GstElement* pipeline) {}
};

struct autovideosink
{
    std::string get_description()
    {
        return
            " videoconvert"
            //"! autovideosink async=false sync=false"
            "! autovideosink"
            ;
    }

    void set_params(GstElement* pipeline) {}
};

struct appsink
{
    GstAppSink* sink;

    const std::string sink_name;

    appsink() :
        sink(nullptr),
        sink_name(get_new_name("sink"))
    {}

    ~appsink()
    {
        gst_object_unref(sink);
    }

    std::string get_description()
    {
        return
            " appsink name=" + sink_name
            ;
    }

    void set_params(GstElement* pipeline)
    {
        //g_print("appsink[%s]::set_params\n", sink_name.c_str());
        sink = GST_APP_SINK(gst_bin_get_by_name(GST_BIN(pipeline), sink_name.c_str()));
        assert(sink);
        //g_object_set(G_OBJECT(sink), "sync", false, nullptr);
        g_object_set(G_OBJECT(sink), "drop", true, nullptr);
        g_object_set(G_OBJECT(sink), "max-buffers", 1, nullptr);
    }
};

struct intervideosink
{
    std::string channel;

    const std::string sink_name;

    intervideosink(const std::string& channel_) :
        channel(channel_),
        sink_name(get_new_name("sink"))
    {}

    std::string get_description()
    {
        return
            " intervideosink name=" + sink_name
            ;
    }

    void set_params(GstElement* pipeline)
    {
        GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), sink_name.c_str());
        assert(sink);
        g_object_set(G_OBJECT(sink), "channel", channel.c_str(), nullptr);
        gst_object_unref(sink);
    }
};

struct tee
{
    const std::string name;

    tee() :
        name(get_new_name("tee"))
    {}

    std::string get_description()
    {
        return " tee name=" + name;
    }

    void set_params(GstElement* pipeline) {}
};

template<typename... Args>
std::string build_pipeline_description(Args&&... args)
{
    // Get descriptions
    std::vector<std::string> pipeline_blocks;
    (pipeline_blocks.push_back(args.get_description()), ...);

    // Connect blocks by queue
    std::string pipeline_str;
    for (int i = 0; i < pipeline_blocks.size(); i++) {
        if (i > 0) pipeline_str += " ! queue ! ";
        pipeline_str += pipeline_blocks[i];
    }

    return pipeline_str;
}

template<typename... Args>
void set_params(GstElement* pipeline, Args&&... args)
{
    // Set parameters
    (args.set_params(pipeline), ...);
}

template<typename... Args>
GstElement* build_pipeline(Args&&... args)
{
    // Build pipeline description
    auto pipeline_str = build_pipeline_description(args...);
    std::cout << pipeline_str << std::endl;

    // Create pipeline
    GstElement* pipeline = gst_parse_launch(pipeline_str.c_str(), nullptr);
    assert(pipeline);

    // Set parameters
    (args.set_params(pipeline), ...);

    return pipeline;
}

template<typename... Args>
GstElement* build_pipeline_and_play(Args&&... args)
{
    auto pipeline = build_pipeline(args...);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    return pipeline;
}


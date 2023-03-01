#pragma once
#include <atomic>
#include <cassert>

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

#include "utils/queue_mt.hpp"

std::string get_new_name(const std::string& base)
{
    static std::atomic<int> id;
    return base + std::to_string(id++);
}

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
        g_object_set(G_OBJECT(sink), "location", location.c_str(), nullptr);
        gst_object_unref(sink);
    }
};

//struct multifilesrc
//{
//    const std::string location;
//
//    const std::string src_name;
//
//    multifilesrc(
//        const std::string& location_
//    ) : 
//        location(location_),
//        src_name(get_new_name("src"))
//    {}
//
//    std::string get_description()
//    {
//        return
//            " multifilesrc name=" + src_name +
//            " ! h264parse"
//            ;
//    }
//
//    void set_params(GstElement* pipeline)
//    {
//        auto src = gst_bin_get_by_name(GST_BIN(pipeline), src_name.c_str());
//        assert(src);
//        g_object_set(G_OBJECT(src), "location", location.c_str(), nullptr);
//        gst_object_unref(src);
//    }
//};

struct videotestsrc
{
    const int width;
    const int height;
    const int pattern;
    const std::string format;

    videotestsrc(
        int width_, int height_, int pattern_ = 0, const std::string& format_ = "I420"
    ) : 
        width(width_),
        height(height_),
        pattern(pattern_),
        format(format_)
    {}

    std::string get_description()
    {
        return
            " videotestsrc pattern=" + std::to_string(pattern) +
            " ! video/x-raw, format=" + format + ", width=" + std::to_string(width) + ", height=" + std::to_string(height)
            ;
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
                     nullptr);
    }
};

template<typename HwConfig>
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
            " vvas_xvcudec name=" + dec_name
            ;
    }

    void set_params(GstElement* pipeline)
    {
        auto dec = gst_bin_get_by_name(GST_BIN(pipeline), dec_name.c_str());
        assert(dec);
        HwConfig::set_decoder_params(dec, dev_idx);
        gst_object_unref(dec);
    }
};

template<typename HwConfig>
struct vvas_scaler
{
    const int dev_idx;
    const int width;
    const int height;
    const std::string format;
    const bool multi_scaler;

    const std::string scaler_name;

    vvas_scaler(
        int dev_idx_,
        int width_, int height_,
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
	        " vvas_xabrscaler name=" + scaler_name;

        if (!multi_scaler)
            desc +=
            " ! video/x-raw, width=" + std::to_string(width) + ", height=" + std::to_string(height) + ", format=" + format;

        return desc;
    }

    void set_params(GstElement* pipeline)
    {
        auto scaler = gst_bin_get_by_name(GST_BIN(pipeline), scaler_name.c_str());
        assert(scaler);
        HwConfig::set_scaler_params(scaler, dev_idx);
        gst_object_unref(scaler);
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
            "! autovideosink async=false sync=false"
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

struct x264enc
{
    const std::string name;

    x264enc() :
        name(get_new_name("enc"))
    {}

    std::string get_description()
    {
        return " x264enc name=" + name;
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


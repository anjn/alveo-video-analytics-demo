#pragma once
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <optional>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
extern "C" {
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include "utils/queue_mt.hpp"

static std::optional<cv::Mat> get_cvmat_from_appsink(GstAppSink* appsink, const cv::Size& size)
{
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (!sample) return {};

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) return {};

    cv::Mat mat = cv::Mat(size.height, size.width, CV_8UC3);

    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_READ);
    std::memcpy(mat.data, info.data, size.width * size.height * 3);
    gst_buffer_unmap(buffer, &info);

    gst_sample_unref(sample);

    return mat;
}

struct app2queue_bgr
{
    using this_type = app2queue_bgr;
    using queue_element_t = cv::Mat;
    using queue_t = queue_mt<queue_element_t>;
    using queue_ptr_t = std::shared_ptr<queue_t>;

    std::vector<std::tuple<GstAppSink**, cv::Size>> appsinks;
    queue_ptr_t queue;

    std::thread thread;

    app2queue_bgr(
        const std::vector<std::tuple<GstAppSink**, cv::Size>>& appsinks_,
        queue_ptr_t& queue_
    ) :
        appsinks(appsinks_), queue(queue_)
    {}

    void start()
    {
        thread = std::thread(&this_type::proc_loop, this);
    }

    void proc_loop()
    {
        while (true)
        {
            std::vector<cv::Mat> mat;
            for (auto& [appsink, size] : appsinks) {
                mat.push_back(get_cvmat_from_appsink(*appsink, size).value());
            }
            proc_buffer(mat);
            queue->push(mat[0]);
        }
    }

    virtual void proc_buffer(std::vector<cv::Mat>& mat) {}
};


struct queue2app_bgr
{
    using this_type = queue2app_bgr;
    using queue_ptr_t = app2queue_bgr::queue_ptr_t;

    queue_ptr_t queue;
    GstAppSrc* appsrc;

    int width;
    int height;

    int gst_buffer_size;
    GstBuffer* last_buffer = nullptr;
    GstClockTime timestamp = 0;
    int fps_n = 15;
    int fps_d = 1;

    queue2app_bgr(
        queue_ptr_t& queue_,
        GstAppSrc* appsrc_,
        int width_,
        int height_
    ) :
        queue(queue_), appsrc(appsrc_), width(width_), height(height_)
    {
        GstVideoInfo info;
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGR, width, height);
        gst_buffer_size = info.size;
        last_buffer = gst_buffer_new_allocate(nullptr, gst_buffer_size, nullptr);
    }

    void start()
    {
        g_signal_connect(appsrc, "need-data", G_CALLBACK(&this_type::need_data), this);

        GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                            "format", G_TYPE_STRING, "BGR",
                                            "width",  G_TYPE_INT, width,
                                            "height", G_TYPE_INT, height,
                                            "framerate", GST_TYPE_FRACTION, fps_n, fps_d,
                                            nullptr);
        gst_app_src_set_caps(appsrc, caps);
        gst_caps_unref(caps);
    }

    static void need_data(GstElement *source, guint size, this_type* obj)
    {
        obj->push_buffer();
    }

    void push_buffer()
    {
        GstBuffer* buffer = nullptr;

        if (queue->size() > 0)
        {
            cv::Mat mat;
            while (queue->size() > 0) mat = queue->pop();

            buffer = gst_buffer_new_allocate(nullptr, gst_buffer_size, nullptr);

            GstMapInfo info;
            gst_buffer_map(buffer, &info, GST_MAP_READ);
            std::memcpy(info.data, mat.data, width * height * 3);
            gst_buffer_unmap(buffer, &info);

            if (last_buffer) gst_buffer_unref(last_buffer);
            last_buffer = gst_buffer_ref(buffer);
        }
        else
        {
            buffer = gst_buffer_copy(last_buffer);
        }

        // Set timestamp
        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(fps_d, GST_SECOND, fps_n);
        timestamp += GST_BUFFER_DURATION(buffer);

        gst_app_src_push_buffer(appsrc, buffer);
    }
};


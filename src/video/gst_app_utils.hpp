#pragma once
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
extern "C" {
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include <gst/rtsp-server/rtsp-server.h>
}
#pragma GCC diagnostic pop

extern "C" {
#include <libswscale/swscale.h>
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include "utils/queue_mt.hpp"
#include "utils/time_utils.hpp"

struct cv_buffer
{
    cv::Mat mat;

    // NV12 buffer info
    int width;
    int height;
    int gst_buffer_size;
    int gst_offset[2];
    int gst_stride[2];

    cv_buffer(int width_, int height_) :
        width(width_), height(height_)
    {
        mat = cv::Mat(height, width, CV_8UC4);
        gst_buffer_size = 0;
    }

    cv_buffer(GstBuffer* buffer, int width_, int height_) :
        cv_buffer(width_, height_)
    {
        set_meta(buffer);
    }

    void set_meta(GstBuffer* buffer)
    {
        buffer = gst_buffer_ref(buffer);

        GstVideoMeta* meta = gst_buffer_get_video_meta(buffer);
        if (!meta) {
            buffer = gst_buffer_make_writable(buffer);
            meta = gst_buffer_add_video_meta(buffer, GST_VIDEO_FRAME_FLAG_NONE, GST_VIDEO_FORMAT_NV12, width, height);
        }

        gst_buffer_size = gst_buffer_get_size(buffer);
        gst_offset[0] = meta->offset[0];
        gst_offset[1] = meta->offset[1];
        gst_stride[0] = meta->stride[0];
        gst_stride[1] = meta->stride[1];

        gst_buffer_unref(buffer);
    }

    SwsContext* create_sws_context_nv12_to_abgr()
    {
        return sws_getContext(width, height, AV_PIX_FMT_NV12,
                              width, height, AV_PIX_FMT_ABGR,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    SwsContext* create_sws_context_abgr_to_nv12()
    {
        return sws_getContext(width, height, AV_PIX_FMT_ABGR,
                              width, height, AV_PIX_FMT_NV12,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    void convert_nv12_to_abgr(SwsContext* sws_ctx, GstBuffer* buffer)
    {
        GstMapInfo info;
        gst_buffer_map(buffer, &info, GST_MAP_READ);

        // NV12
        uint8_t* src[2] = { info.data + gst_offset[0], info.data + gst_offset[1] };
        int linesize[2] = { gst_stride[0], gst_stride[1] };

        // ABGR
        uint8_t* dest[1] = { mat.data };
        int dest_linesize[1] = { (int) width * 4 };

        sws_scale(sws_ctx, src, linesize, 0, height, dest, dest_linesize);

        gst_buffer_unmap(buffer, &info);
    }

    void convert_abgr_to_nv12(SwsContext* sws_ctx, GstBuffer* buffer)
    {
        buffer = gst_buffer_make_writable(buffer);
        //buffer = gst_buffer_make_writable(gst_buffer_ref(buffer));

        GstMapInfo info;
        gst_buffer_map(buffer, &info, GST_MAP_WRITE);

        // NV12
        uint8_t* src[2] = { info.data + gst_offset[0], info.data + gst_offset[1] };
        int linesize[2] = { gst_stride[0], gst_stride[1] };

        // ABGR
        uint8_t* dest[1] = { mat.data };
        int dest_linesize[1] = { (int) width * 4 };

        sws_scale(sws_ctx, dest, dest_linesize, 0, height, src, linesize);

        gst_buffer_unmap(buffer, &info);
    }
};

static auto create_app_queue()
{
    return std::make_shared<queue_mt<std::shared_ptr<cv_buffer>>>();
}

struct app2queue
{
    using queue_element_t = std::shared_ptr<cv_buffer>;
    using queue_t = queue_mt<queue_element_t>;
    using queue_ptr_t = std::shared_ptr<queue_t>;

    GstAppSink*& appsink;
    queue_ptr_t queue;
    int width;
    int height;

    SwsContext* sws_ctx;
    std::thread thread;

    app2queue(
        GstAppSink*& appsink_,
        queue_ptr_t& queue_,
        int width_,
        int height_
    ) :
        appsink(appsink_), queue(queue_),
        width(width_), height(height_),
        sws_ctx(nullptr)
    {}

    void start()
    {
        thread = std::thread(&app2queue::proc_loop, this);
    }

    void proc_loop()
    {
        while (true)
        {
            GstSample* sample = gst_app_sink_pull_sample(appsink);
            if (!sample) break;

            GstBuffer* buffer = gst_sample_get_buffer(sample);
            if (!buffer) break;

            //g_print("new buffer @ app2queue\n");

            auto cv_buf = std::make_shared<cv_buffer>(buffer, width, height);
            if (!sws_ctx) sws_ctx = cv_buf->create_sws_context_nv12_to_abgr();
            cv_buf->convert_nv12_to_abgr(sws_ctx, buffer);

            proc_buffer(cv_buf->mat);

            if (queue->size() > 10)
                //std::cout << "app2queue : queue is full " << queue->size() << std::endl;
                ;
            else
                queue->push(cv_buf);

            //gst_buffer_unref(buffer);
            gst_sample_unref(sample);
        }
    }

    virtual void proc_buffer(cv::Mat& mat) { }
};

struct queue2app
{
    using queue_element_t = std::shared_ptr<cv_buffer>;
    using queue_t = queue_mt<queue_element_t>;
    using queue_ptr_t = std::shared_ptr<queue_t>;

    using this_type = queue2app;

    queue_ptr_t queue;
    GstAppSrc* appsrc;

    SwsContext* sws_ctx = nullptr;
    GstBuffer* last_gst_buffer = nullptr;
    std::thread thread;
    GstClockTime timestamp = 0;
    int fps_n = 30;
    int fps_d = 1;
    bool set_caps = true;

    queue2app(
        queue_ptr_t& queue_,
        GstAppSrc* appsrc_
    ) :
        queue(queue_), appsrc(appsrc_)
    {}

    void start()
    {
        g_signal_connect(appsrc, "need-data", G_CALLBACK(&this_type::need_data), this);
    }

    static void need_data(GstElement *source, guint size, this_type* obj)
    {
        obj->push_buffer();
    }

    void push_buffer()
    {
        // Get last frame
        queue_element_t cv_buf;
        while (queue->size() > 0 || (!last_gst_buffer && !cv_buf)) {
            cv_buf = queue->pop();
        }

        if (set_caps && cv_buf)
        {
            GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                                "format", G_TYPE_STRING, "NV12",
                                                "width",  G_TYPE_INT, cv_buf->width,
                                                "height", G_TYPE_INT, cv_buf->height,
                                                "framerate", GST_TYPE_FRACTION, fps_n, fps_d,
                                                nullptr);
            gst_app_src_set_caps(appsrc, caps);
            gst_caps_unref(caps);
            set_caps = false;
        }

        GstBuffer* buffer = nullptr;

        if (cv_buf)
        {
            proc_buffer(cv_buf->mat);

            buffer = gst_buffer_new_allocate(nullptr, cv_buf->gst_buffer_size, nullptr);

            if (!sws_ctx) sws_ctx = cv_buf->create_sws_context_abgr_to_nv12();
            cv_buf->convert_abgr_to_nv12(sws_ctx, buffer);

            if (last_gst_buffer) gst_buffer_unref(last_gst_buffer);
            last_gst_buffer = gst_buffer_ref(buffer);
        }
        else
        {
            assert(last_gst_buffer);
            buffer = gst_buffer_copy(last_gst_buffer);
        }

        // Set timestamp
        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(fps_d, GST_SECOND, fps_n);
        timestamp += GST_BUFFER_DURATION(buffer);

        gst_app_src_push_buffer(appsrc, buffer);
    }

    virtual void proc_buffer(cv::Mat& mat) {}
};


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
}

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
    using queue_element_t = std::vector<cv::Mat>;
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
            queue->push(mat);
        }
    }

    virtual void proc_buffer(std::vector<cv::Mat>& mat) {}
};


#pragma once

#include "ml/bcc/bcc_client.hpp"
#include "video/gst_app_bgr_utils.hpp"
#include "utils/time_utils.hpp"

#include "metrics.hpp"

inline std::string bcc_server_address;
inline fps_counter bcc_fps(1000);

struct app2queue_bcc : app2queue_bgr
{
    bcc_client::queue_t result_queue;
    bcc_client client;
    cv::Mat heatmap;
    int count;

    std::shared_ptr<metrics_client_influx_db> metrics_client;

    app2queue_bcc(
        std::vector<std::tuple<GstAppSink**, cv::Size>> appsinks_,
        queue_ptr_t& queue_
    ) :
        app2queue_bgr(appsinks_, queue_),
        result_queue(bcc_client::create_result_queue()),
        client(bcc_server_address, result_queue),
        count(0)
    {}

    virtual void proc_buffer(std::vector<cv::Mat>& mats) override
    {
        auto mat = mats[0];

        // Request ML inference if client is not busy
        if (!client.is_busy())
        {
            client.request(mat.clone());
        }

        // Get ML inference result from queue
        while (result_queue->size() > 0)
        {
            std::tie(heatmap, count) = result_queue->pop();

            if (metrics_client)
                metrics_client->push(count);
        }

        // Draw heatmap
        if (!heatmap.empty())
        {
            for (int y = 0; y < mat.rows - 8; y++) {
                for (int x = 0; x < mat.cols; x++) {
                    auto& p = mat.at<cv::Vec3b>(y, x);
                    auto& ph = heatmap.at<cv::Vec4b>(y / 8 , x / 8);
                    float pa = ph[3] / 255.0;
                    for (int c = 0; c < 3; c++) {
                        p[c] = p[c] * (1 - pa) + ph[c] * pa;
                    }
                }
            }
        }

        // Draw text
        cv::putText(mat, std::to_string(int(count)), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(0, 0, 0), 14);
        cv::putText(mat, std::to_string(int(count)), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(60, 240, 60), 6);

        //if (metrics_client)
        //{
        //    auto id = metrics_client->get_id();
        //    cv::putText(mat, id, cv::Point(24, mat.rows - 36), cv::FONT_HERSHEY_DUPLEX, 2, cv::Scalar(0, 0, 0), 14);
        //    cv::putText(mat, id, cv::Point(24, mat.rows - 36), cv::FONT_HERSHEY_DUPLEX, 2, cv::Scalar(255, 255, 255), 6);
        //}
    }
};



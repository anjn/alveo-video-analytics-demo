#pragma once
#include <memory>

#include "ml/base/inf_client.hpp"
#include "ml/bcc/bcc_message.hpp"

struct bcc_message_adapter
{
    using request_t = cv::Mat;
    using result_t = std::tuple<cv::Mat, int>;
    using request_message_t = bcc_request_message;
    using result_message_t = bcc_result_message;

    request_message_t create_request(request_t mat)
    {
        // Create request
        bcc_request_message r;

        if (mat.cols > 960)
        {
            cv::Mat dst;
            int dst_w = 960;
            int dst_h = mat.rows * dst_w / mat.cols;
            cv::resize(mat, dst, cv::Size(dst_w, dst_h), 0, 0, cv::INTER_NEAREST);
            mat = dst;
        }

        r.image_w = mat.cols;
        r.image_h = mat.rows;
        r.image_c = 4;
        r.image_stride = mat.step;
        int size = mat.step * r.image_h;
        r.image.resize(size);
        std::memcpy(r.image.data(), mat.data, size);

        return r;
    }

    result_t create_result(const result_message_t& result)
    {
        cv::Mat heatmap(result.map_h, result.map_w, CV_8UC4);

        assert(result.map_w * result.map_h == result.map.size());

        // TODO optimize color calculation
        for (int y = 0; y < result.map_h; y++) {
            for (int x = 0; x < result.map_w; x++) {
                double v = result.map[y * result.map_w + x] / 127.0;
                double va = pow(v, 0.4);
                cv::Vec4b p;
                p[0] = 255;
                p[1] = std::min(255, (int)(v * 250));
                p[2] = 0;
                p[3] = std::min(255, (int)(va * 224));

                heatmap.at<cv::Vec4b>(y, x) = p;
            }
        }

        return std::make_tuple(heatmap, result.count);
    }
};

using bcc_client = inf_client<bcc_message_adapter>;


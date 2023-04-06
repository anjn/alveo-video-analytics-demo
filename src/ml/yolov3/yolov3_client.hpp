#pragma once
#include <memory>
#include <tuple>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include "ml/base/inf_client.hpp"
#include "ml/yolov3/yolov3_message.hpp"

struct yolov3_message_adapter
{
    using request_t = cv::Mat;
    using result_t = yolov3_result_message;
    using request_message_t = yolov3_request_message;
    using result_message_t = yolov3_result_message;

    request_message_t create_request(const request_t& mat)
    {
        // Create request
        yolov3_request_message req_obj;
        req_obj.rows = mat.rows;
        req_obj.cols = mat.cols;
        req_obj.mat.resize(mat.rows * mat.cols * 3);
        std::memcpy(req_obj.mat.data(), mat.data, mat.rows * mat.cols * 3);

        return req_obj;
    }

    result_t create_result(const result_message_t& result)
    {
        return result;
    }
};

using yolov3_client = inf_client<yolov3_message_adapter>;

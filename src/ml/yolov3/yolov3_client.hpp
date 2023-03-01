#pragma once
#include <memory>

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
        //std::cout << "yolov3_message_adapter::create_request" << std::endl;
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
        //std::cout << "yolov3_message_adapter::create_result" << std::endl;
        return result;
    }
};

using yolov3_client = inf_client<yolov3_message_adapter>;

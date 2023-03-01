#pragma once
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include <vitis/ai/yolov3.hpp>

#include "ml/base/inf_server.hpp"
#include "ml/yolov3/yolov3_message.hpp"

struct yolov3_model
{
    using request_t = yolov3_request_message;
    using result_t = yolov3_result_message;

    std::unique_ptr<vitis::ai::YOLOv3> model;

    yolov3_model(const std::vector<std::string>& xmodel_paths) :
        model(vitis::ai::YOLOv3::create(xmodel_paths[0]))
    {}

    std::vector<result_t> run(const std::vector<request_t>& requests)
    {
        std::cout << "yolov3_model::run" << std::endl;
        std::vector<cv::Mat> req_batch;

        for (const auto& req_obj : requests)
        {
            cv::Mat mat(req_obj.rows, req_obj.cols, CV_8UC3);
            std::memcpy(mat.data, req_obj.mat.data(), req_obj.rows * req_obj.cols * 3);
            req_batch.push_back(mat);
        }

        const int batch_size = req_batch.size();
        auto res_batch = model->run(req_batch);

        std::vector<result_t> results;

        for (auto i = 0u; i < batch_size; i++)
        {
            // Create response
            auto& result = results.emplace_back();

            for (auto& box : res_batch[i].bboxes)
            {
                yolov3_bbox r;
                r.label = box.label;
                r.prob = box.score;
                r.x = box.x;
                r.y = box.y;
                r.width = box.width;
                r.height = box.height;
                result.detections.push_back(r);
            }
        }

        return results;
    }
};

using yolov3_server = inf_server<yolov3_model>;

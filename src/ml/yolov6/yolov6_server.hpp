#pragma once
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include <vitis/ai/yolov6.hpp>

#include "ml/base/inf_server.hpp"
#include "ml/yolo_common/yolo_message.hpp"

struct yolov6_model
{
    using request_t = yolo_request_message;
    using result_t = yolo_result_message;

    std::unique_ptr<vitis::ai::YOLOv6> model;

    yolov6_model(const std::vector<std::string>& xmodel_paths) :
        model(vitis::ai::YOLOv6::create(xmodel_paths[0]))
    {}

    std::vector<result_t> run(const std::vector<request_t>& requests)
    {
        std::cout << "yolov6_model::run " << std::to_string(requests.size()) << std::endl;
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
            auto& rows = requests[i].rows;
            auto& cols = requests[i].cols;

            for (auto& res : res_batch[i].bboxes)
            {
                yolo_bbox r;
                r.label = res.label;
                r.prob = res.score;
                auto& box = res.box;
                r.x = box[0] / cols;
                r.y = box[1] / rows;
                r.width = (box[2] - box[0]) / cols;
                r.height = (box[3] - box[1]) / rows;
                result.detections.push_back(r);
            }
        }

        return results;
    }
};

using yolov6_server = inf_server<yolov6_model>;


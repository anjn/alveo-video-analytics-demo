#pragma once
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include <vitis/ai/classification.hpp>
#include <vitis/ai/vehicleclassification.hpp>

#include "ml/base/inf_server.hpp"
#include "ml/carclassification/carclassification_message.hpp"

struct carclassification_model
{
    using request_t = carclassification_request_message;
    using result_t = carclassification_result_message;

    std::unique_ptr<vitis::ai::Classification> color_model;
    std::unique_ptr<vitis::ai::VehicleClassification> make_model;
    std::unique_ptr<vitis::ai::VehicleClassification> type_model;

    carclassification_model(const std::vector<std::string>& xmodel_paths) :
        color_model(vitis::ai::Classification::create(xmodel_paths[0])),
        make_model(vitis::ai::VehicleClassification::create(xmodel_paths[1])),
        type_model(vitis::ai::VehicleClassification::create(xmodel_paths[2]))
    {}

    std::vector<result_t> run(const std::vector<request_t>& requests)
    {
        std::cout << "carclassification_model::run" << std::endl;
        std::vector<cv::Mat> req_batch;

        for (const auto& req_obj : requests)
        {
            cv::Mat mat(req_obj.rows, req_obj.cols, CV_8UC3);
            std::memcpy(mat.data, req_obj.mat.data(), req_obj.rows * req_obj.cols * 3);
            req_batch.push_back(mat);
        }

        const int batch_size = req_batch.size();
        auto color_results = color_model->run(req_batch);
        auto make_results = make_model->run(req_batch);
        auto type_results = type_model->run(req_batch);

        std::vector<result_t> results;

        for (auto i = 0u; i < batch_size; i++)
        {
            // Create response
            auto& result = results.emplace_back();

            for (auto& r : color_results[i].scores)
            {
                result.color.score = r.score;
                result.color.label_id = r.index;
                result.color.label = color_results[i].lookup(r.index);
                result.color.label.pop_back(); // remove last ','
            }

            for (auto& r : make_results[i].scores)
            {
                result.make.score = r.score;
                result.make.label_id = r.index;
                result.make.label = make_results[i].lookup(r.index);
                result.make.label.pop_back(); // remove last ','
            }

            for (auto& r : type_results[i].scores)
            {
                result.type.score = r.score;
                result.type.label_id = r.index;
                result.type.label = type_results[i].lookup(r.index);
                result.type.label.pop_back(); // remove last ','
            }
        }

        return results;
    }
};

using carclassification_server = inf_server<carclassification_model>;


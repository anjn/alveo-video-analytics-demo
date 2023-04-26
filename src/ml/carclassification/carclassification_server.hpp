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
    //std::unique_ptr<vitis::ai::VehicleClassification> type_model;
    std::unique_ptr<vitis::ai::Classification> type_model;

    carclassification_model(const std::vector<std::string>& xmodel_paths) :
        color_model(vitis::ai::Classification::create(xmodel_paths[0])),
        make_model(vitis::ai::VehicleClassification::create(xmodel_paths[1])),
        //type_model(vitis::ai::VehicleClassification::create(xmodel_paths[2]))
        type_model(vitis::ai::Classification::create(xmodel_paths[2]))
    {}

    std::vector<result_t> run(const std::vector<request_t>& requests)
    {
        std::cout << "carclassification_model::run " << std::to_string(requests.size()) << std::endl;
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
                auto& color = result.color.emplace_back();
                color.score = r.score;
                color.label_id = r.index;
                color.label = color_results[i].lookup(r.index);
                color.label.pop_back(); // remove last ','
            }

            for (auto& r : make_results[i].scores)
            {
                auto& make = result.make.emplace_back();
                make.score = r.score;
                make.label_id = r.index;
                make.label = make_results[i].lookup(r.index);
                make.label.pop_back(); // remove last ','
            }

            for (auto& r : type_results[i].scores)
            {
                auto& type = result.type.emplace_back();
                type.score = r.score;
                type.label_id = r.index;
                type.label = type_results[i].lookup(r.index);
                type.label.pop_back(); // remove last ','
            }
        }

        return results;
    }
};

using carclassification_server = inf_server<carclassification_model>;


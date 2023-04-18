#include "ml/carclassification/carclassification_client.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include <arg/arg.h>
#include <spdlog/spdlog.h>

#include "ml/yolo_common/yolo_client.hpp"
#include "ml/yolov6/yolov6_server.hpp"

int main(int argc, char** argv)
{
    arg_begin("VIDEO", 1, 1);
    arg_s(xmodel, "/usr/share/vitis_ai_library/models/yolov6m_pt/yolov6m_pt.xmodel", "Path of xmodel file");
    arg_d(prob_threshold, 0.6, "Probability threshold");
    arg_i(extract_label, 2, "Extract label (default: car)");
    arg_d(min_size, 0.1, "Minimum size");
    arg_i(interval, 60, "Extract interval in frame");
    arg_s(output, "out/img{:08}.jpg", "Output filename");
    arg_i(start_index, 0, "Output index");
    arg_end;

    // arguments
    std::string video = args[0];

    // yolov6 server
    inf_server_config conf;
    conf.name = "yolov6";
    conf.xmodels = { xmodel };
    conf.address = "tcp://127.0.0.1:5556";
    conf.max_batch_latency = 100;
    conf.max_batch_size = 3;
    conf.num_workers = 1;

    auto server = std::make_shared<yolov6_server>(conf);
    server->start(true);

    // client
    auto result_queue = yolo_client::create_result_queue();
    yolo_client client(conf.address, result_queue);

    // video input
    cv::VideoCapture cap;
    cap.open(video);
    if (!cap.isOpened()) abort();

    int frame = 0;
    int output_index = start_index;

    while (true)
    {
        cv::Mat img;
        for (int i = 0; i < interval; i++) {
            cap >> img;
            frame++;
        }

        if (img.empty()) break;
        
        spdlog::info("Processing frame {}", frame);

        client.request(img);
        const auto& result = result_queue->pop();

        for (auto& det : result.detections)
        {
            if (det.label == extract_label && det.prob >= prob_threshold && (det.width >= min_size || det.height >= min_size))
            {
                auto crop =  crop_resize_for_carclassification(img, det, 1.0);                
                auto fn = fmt::format(output, output_index++);
                cv::imwrite(fn, crop);
                spdlog::info("    Output to {}", fn);
            }
        }
    }
}




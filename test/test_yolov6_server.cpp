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

#include "ml/yolo_common/yolo_client.hpp"
#include "ml/yolov6/yolov6_server.hpp"

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(xmodel, "/usr/share/vitis_ai_library/models/yolov6m_pt/yolov6m_pt.xmodel", "Path of xmodel file");
    arg_s(image, "/workspace/demo/samples/The_million_march_man.jpg", "Test image");
    arg_d(prob_threshold, 0.3, "Probability threshold.");
    arg_end;

    // yolov6
    inf_server_config conf;
    conf.name = "yolov6";
    conf.xmodels = { xmodel };
    conf.address = "tcp://127.0.0.1:5556";
    conf.max_batch_latency = 100;
    conf.max_batch_size = 3;
    conf.num_workers = 1;

    auto server = std::make_shared<yolov6_server>(conf);
    server->start(true);

    cv::Mat img = cv::imread(image);

    auto result_queue = yolo_client::create_result_queue();
    yolo_client client(conf.address, result_queue);

    client.request(img);
    const auto& result = result_queue->pop();

    std::cout << "result: " << result.detections.size() << std::endl;

    for (auto& det : result.detections)
    {
        std::cout << det.label << "\t" << (int)(det.prob * 100) << "\t" <<
            det.x << "\t" << det.y << "\t" << det.width << "\t" << det.height << std::endl;
        if (det.prob >= prob_threshold)
        {
            cv::rectangle(img, cv::Point(det.x * img.cols, det.y * img.rows),
                          cv::Point((det.x + det.width) * img.cols, (det.y + det.height) * img.rows),
                          cv::Scalar(0, 255, 0), 1, 1, 0);
        }
    }

    cv::namedWindow("title", cv::WINDOW_NORMAL);
    cv::imshow("title", img);
    cv::waitKey();

    //cv::imwrite("test.png", img);
}



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

#include "ml/yolov3/yolov3_server.hpp"
#include "ml/yolov3/yolov3_client.hpp"

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(xmodel, "/usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC/yolov3_voc_tf/yolov3_voc_tf.xmodel", "Path of xmodel file");
    arg_s(image, "/workspace/Vitis-AI/examples/Vitis-AI-Library/samples/yolov3/sample_yolov3.jpg", "Test image");
    arg_end;

    // yolov3
    inf_server_config conf;
    conf.name = "yolov3";
    conf.xmodels = { xmodel };
    conf.address = "tcp://127.0.0.1:5556";
    conf.max_batch_latency = 100;
    conf.max_batch_size = 3;
    conf.num_workers = 1;

    auto server = std::make_shared<yolov3_server>(conf);
    server->start(true);

    cv::Mat img = cv::imread(image);

    auto result_queue = yolov3_client::create_result_queue();
    yolov3_client client(conf.address, result_queue);

    client.request(img);
    auto result = result_queue->pop();

    std::cout << "result: " << result.detections.size() << std::endl;

    for (auto& det : result.detections)
    {
        std::cout << det.label << "\t" << det.prob << "\t" << det.x << "\t" << det.y << std::endl;
        if (det.prob >= 0.5)
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


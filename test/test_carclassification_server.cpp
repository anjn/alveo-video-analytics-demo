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

#include "ml/carclassification/carclassification_server.hpp"
#include "ml/carclassification/carclassification_client.hpp"
#include "ml/yolov3/yolov3_server.hpp"
#include "ml/yolov3/yolov3_client.hpp"

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(yolo_xmodel, "/usr/share/vitis_ai_library/models/yolov3_voc_tf/yolov3_voc_tf.xmodel", "Path of xmodel file");
    arg_s(color_xmodel, "/usr/share/vitis_ai_library/models/chen_color_resnet18_pt/chen_color_resnet18_pt.xmodel", "Path of xmodel file");
    arg_s(make_xmodel, "/usr/share/vitis_ai_library/models/vehicle_make_resnet18_pt/vehicle_make_resnet18_pt.xmodel", "Path of xmodel file");
    arg_s(type_xmodel, "/usr/share/vitis_ai_library/models/vehicle_type_resnet18_pt/vehicle_type_resnet18_pt.xmodel", "Path of xmodel file");
    arg_s(image, "/workspace/demo/samples/The_million_march_man.jpg", "Test image");
    arg_end;

    // yolov3
    inf_server_config y_conf;
    y_conf.name = "yolov3";
    y_conf.xmodels = { yolo_xmodel };
    y_conf.address = "tcp://127.0.0.1:5556";
    y_conf.max_batch_latency = 100;
    y_conf.max_batch_size = 14;
    y_conf.num_workers = 1;

    // carclassification
    inf_server_config c_conf;
    c_conf.name = "carclassification";
    c_conf.xmodels = { color_xmodel, make_xmodel, type_xmodel };
    c_conf.address = "tcp://127.0.0.1:5557";
    c_conf.max_batch_latency = 100;
    c_conf.max_batch_size = 14;
    c_conf.num_workers = 1;

    auto y_server = std::make_shared<yolov3_server>(y_conf);
    y_server->start(true);

    auto c_server = std::make_shared<carclassification_server>(c_conf);
    c_server->start(true);

    cv::Mat img = cv::imread(image);

    auto y_result_queue = yolov3_client::create_result_queue();
    yolov3_client y_client(y_conf.address, y_result_queue);

    auto c_result_queue = carclassifcation_client::create_result_queue();
    carclassifcation_client c_client(c_conf.address, c_result_queue);

    y_client.request(img);
    auto result = y_result_queue->pop();

    std::cout << "result: " << result.detections.size() << std::endl;

    for (auto& det : result.detections)
    {
        std::cout << det.label << "\t" << det.prob << "\t" << det.x << "\t" << det.y << std::endl;
        if (det.prob >= 0.5)
        {
            auto car = crop_resize_for_carclassification(img, det);

            c_client.request(car);
            auto result = c_result_queue->pop();

            std::cout << vehicle_color_labels[result.color.label_id] << std::endl;
            std::cout << vehicle_make_labels[result.make.label_id] << std::endl;
            std::cout << vehicle_type_labels[result.type.label_id] << std::endl;

            cv::namedWindow("title", cv::WINDOW_NORMAL);
            cv::imshow("title", car);
            cv::waitKey();
        }
    }
}


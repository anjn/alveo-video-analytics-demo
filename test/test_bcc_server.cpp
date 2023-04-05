


#include <cassert>
#include <cmath>
#include <cstdio>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include <arg/arg.h>

#include "ml/bcc/bcc_server.hpp"
#include "ml/bcc/bcc_client.hpp"

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(xmodel, "/usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC/bcc_pt/bcc_pt.xmodel", "Path of xmodel file");
    arg_s(image, "/workspace/Vitis-AI/examples/Vitis-AI-Library/samples/bcc/sample_bcc.jpg", "Test image");
    arg_end;

    // bcc
    inf_server_config conf;
    conf.name = "bcc";
    conf.xmodels = { xmodel };
    conf.address = "tcp://127.0.0.1:5556";
    conf.max_batch_latency = 100;
    conf.max_batch_size = 3;
    conf.num_workers = 1;

    auto server = std::make_shared<bcc_server>(conf);
    server->start(true);

    cv::Mat mat = cv::imread(image);

    cv::Mat img;
    cv::cvtColor(mat, img, cv::COLOR_BGR2BGRA);

    auto result_queue = bcc_client::create_result_queue();
    bcc_client client(conf.address, result_queue);

    client.request(img);
    auto [heatmap, count] = result_queue->pop();

    std::cout << "count: " << count << std::endl;

    // Draw heatmap
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            auto& p = img.at<cv::Vec4b>(y, x);
            auto& ph = heatmap.at<cv::Vec4b>(y / 8 , x / 8);
            float pa = ph[3] / 255.0;
            for (int c = 0; c < 3; c++) {
                p[c] = p[c] * (1 - pa) + ph[c] * pa;
            }
        }
    }

    //cv::imwrite("test.png", heatmap);
    cv::namedWindow("title", cv::WINDOW_NORMAL);
    cv::imshow("title", img);
    cv::waitKey();
}


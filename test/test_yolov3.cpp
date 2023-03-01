#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include <vitis/ai/yolov3.hpp>

#include <arg/arg.h>

#include "utils/time_utils.hpp"

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(xmodel, "/usr/share/vitis_ai_library/models/u55c-u50lv-DPUCAHX8H-DWC/yolov3_voc_tf/yolov3_voc_tf.xmodel", "Path of xmodel file");
    arg_s(image, "/workspace/Vitis-AI/examples/Vitis-AI-Library/samples/yolov3/sample_yolov3.jpg", "Test image");
    arg_end;

    auto yolo = vitis::ai::YOLOv3::create(xmodel);
    cv::Mat img = cv::imread(image);

    std::vector<cv::Mat> imgs;
    imgs.push_back(img);

    stop_watch sw;
    auto results = yolo->run(imgs);
    auto time = sw.get_time_ns();
    std::cout << "latency = " << (time / 1e6) << "ms" << std::endl;
    std::cout << "fps = " << (1 * 1e9 / time) << std::endl;

    for(auto &box : results[0].bboxes){
        int label = box.label;
        float xmin = box.x * img.cols + 1;
        float ymin = box.y * img.rows + 1;
        float xmax = xmin + box.width * img.cols;
        float ymax = ymin + box.height * img.rows;
        if(xmin < 0.) xmin = 1.;
        if(ymin < 0.) ymin = 1.;
        if(xmax > img.cols) xmax = img.cols;
        if(ymax > img.rows) ymax = img.rows;
        float confidence = box.score;
        if (confidence >= 0.5)
        {
            std::cout << "RESULT: " << label << "\t" << xmin << "\t" << ymin << "\t"
                << (xmax - xmin) << "\t" << (ymax - ymin) << "\t" << confidence << "\n";
            rectangle(img, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 255, 0), 1, 1, 0);
        }
    }

    cv::namedWindow("title", cv::WINDOW_NORMAL);
    cv::imshow("title", img);
    cv::waitKey();
}

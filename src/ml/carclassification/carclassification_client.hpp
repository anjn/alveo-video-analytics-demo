#pragma once
#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include "ml/base/inf_client.hpp"
#include "ml/carclassification/carclassification_message.hpp"
#include "ml/yolo_common/yolo_message.hpp"

struct carclassification_message_adapter
{
    using request_t = cv::Mat;
    using result_t = carclassification_result_message;
    using request_message_t = carclassification_request_message;
    using result_message_t = carclassification_result_message;

    request_message_t create_request(const request_t& mat)
    {
        // Create request
        carclassification_request_message req_obj;
        req_obj.rows = mat.rows;
        req_obj.cols = mat.cols;
        req_obj.mat.resize(mat.rows * mat.cols * 3);
        std::memcpy(req_obj.mat.data(), mat.data, mat.rows * mat.cols * 3);

        return req_obj;
    }

    result_t create_result(const result_message_t& result)
    {
        return result;
    }
};

using carclassifcation_client = inf_client<carclassification_message_adapter>;

// Labels
inline std::string vehicle_color_labels[] = { "beige", "black", "blue", "brown", "gold", "green", "grey", "orange", "pink", "purple", "red", "silver", "tan", "white", "yellow", };
//inline std::string vehicle_color_labels[] = { "black", "blue", "grey", "red", "silver", "white", "yellow" };

inline std::string vehicle_make_labels[] = { "acura", "audi", "bmw", "buick", "cadillac", "chevrolet", "chrysler", "dodge", "ford", "gmc", "honda", "hummer", "hyundai", "infiniti", "isuzu", "jaguar", "jeep", "kia", "landrover", "lexus", "lincoln", "mazda", "mercedes_benz", "mercury", "mini", "mitsubishi", "nissan", "oldsmobile", "plymouth", "pontiac", "porsche", "saab", "saturn", "scion", "subaru", "suzuki", "toyota", "volkswagen", "volvo", };

inline std::string vehicle_type_labels[] = { "SUV", "Convertible", "Coupe", "Hatchback", "Limousine", "Minivan", "Sedan", };
//inline std::string vehicle_type_labels[] = { "Convertible", "Hatchback", "Minivan", "SUV", "Sedan" };

// Utils
static cv::Mat crop_resize_for_carclassification(
    const cv::Mat& img,
    const cv::Rect& box,
    float scale_factor = 1.0
) {
    int x = box.x;
    int y = box.y;
    int width = box.width;
    int height = box.height;
    int size = std::max(width, height);

    size = size * scale_factor;

    // ROI region
    x = std::min(std::max(x + width / 2 - size / 2, 0), img.cols - 1);
    y = std::min(std::max(y + height / 2 - size / 2, 0), img.rows - 1);
    width = std::min(x + size, img.cols) - x;
    height = std::min(y + size, img.rows) - y;

    assert(0 <= x && x < img.cols);
    assert(0 <= y && y < img.rows);
    assert(0 < width && x + width <= img.cols);
    assert(0 < height && y + height <= img.rows);

    // Crop and resize
    auto car = cv::Mat(224, 224, CV_8UC3);
    cv::Rect roi { x, y, width, height };
    //std::cout << img.size << std::endl;
    //std::cout << roi << std::endl;
    cv::resize(img(roi), car, cv::Size(car.cols, car.rows), 0, 0, cv::INTER_LINEAR);

    return car;
}

static cv::Mat crop_resize_for_carclassification(
    const cv::Mat& img,
    const yolo_bbox& det,
    float scale_factor = 1.0
) {
    cv::Rect box {
        int(det.x * img.cols), int(det.y * img.rows),
        int(det.width * img.cols), int(det.height * img.rows)
    };
    return crop_resize_for_carclassification(img, box, scale_factor);
}

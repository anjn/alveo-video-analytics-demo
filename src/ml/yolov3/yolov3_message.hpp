#pragma once
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "msgpack.hpp"
#pragma GCC diagnostic pop

struct yolov3_bbox {
    int label;
    float prob;
    float x;
    float y;
    float width;
    float height;

    MSGPACK_DEFINE(label, prob, x, y, width, height);
};

struct yolov3_request_message {
    std::vector<uint8_t> mat; // BGR matrix
    int rows;
    int cols;
    
    MSGPACK_DEFINE(mat, rows, cols);
};

struct yolov3_result_message {
    std::vector<yolov3_bbox> detections;

    MSGPACK_DEFINE(detections);
};

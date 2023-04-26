#pragma once
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "msgpack.hpp"
#pragma GCC diagnostic pop

struct classification_result
{
    float score;
    int label_id;
    std::string label;

    MSGPACK_DEFINE(score, label_id, label);
};

struct carclassification_request_message
{
    std::vector<uint8_t> mat; // BGR matrix
    int rows;
    int cols;
    
    MSGPACK_DEFINE(mat, rows, cols);
};

struct carclassification_result_message
{
    std::vector<classification_result> color;
    std::vector<classification_result> make;
    std::vector<classification_result> type;

    MSGPACK_DEFINE(color, make, type);
};

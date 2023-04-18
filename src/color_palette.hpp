#pragma once
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

struct color_palette
{
    std::vector<cv::Scalar> values;

    auto operator[](unsigned int index)
    {
        return values[index % values.size()];
    }

    static color_palette get_default()
    {
        color_palette palette;
        // BGR order
        // Generated with https://medialab.github.io/iwanthue/
        palette.values.push_back(cv::Scalar(0x58, 0x98, 0xe4));
        palette.values.push_back(cv::Scalar(0xea, 0x56, 0x92));
        palette.values.push_back(cv::Scalar(0x46, 0xd5, 0x50));
        palette.values.push_back(cv::Scalar(0xe1, 0x4d, 0xca));
        palette.values.push_back(cv::Scalar(0x33, 0xce, 0x88));
        palette.values.push_back(cv::Scalar(0xbb, 0x31, 0xe5));
        palette.values.push_back(cv::Scalar(0x7f, 0xda, 0x5a));
        palette.values.push_back(cv::Scalar(0xa6, 0x3c, 0xc5));
        palette.values.push_back(cv::Scalar(0x3a, 0xd0, 0xbc));
        palette.values.push_back(cv::Scalar(0xea, 0x6a, 0x69));
        palette.values.push_back(cv::Scalar(0x25, 0xca, 0xd6));
        palette.values.push_back(cv::Scalar(0xed, 0x7e, 0x41));
        palette.values.push_back(cv::Scalar(0x31, 0xb8, 0xe9));
        palette.values.push_back(cv::Scalar(0xcd, 0x6d, 0x6c));
        palette.values.push_back(cv::Scalar(0x5f, 0xcb, 0x7b));
        palette.values.push_back(cv::Scalar(0xd2, 0x66, 0xe9));
        palette.values.push_back(cv::Scalar(0x30, 0x93, 0x42));
        palette.values.push_back(cv::Scalar(0xc5, 0x60, 0xa6));
        palette.values.push_back(cv::Scalar(0x40, 0xb5, 0x9d));
        palette.values.push_back(cv::Scalar(0xea, 0x8d, 0x9e));
        palette.values.push_back(cv::Scalar(0x26, 0x8b, 0xe7));
        palette.values.push_back(cv::Scalar(0xe9, 0x93, 0x4c));
        palette.values.push_back(cv::Scalar(0x1e, 0x44, 0xe8));
        palette.values.push_back(cv::Scalar(0xaa, 0xdc, 0x42));
        palette.values.push_back(cv::Scalar(0x8b, 0x41, 0xe0));
        palette.values.push_back(cv::Scalar(0x66, 0xa7, 0x4c));
        palette.values.push_back(cv::Scalar(0x67, 0x3c, 0xdf));
        palette.values.push_back(cv::Scalar(0xca, 0xd2, 0x5e));
        palette.values.push_back(cv::Scalar(0x43, 0x3a, 0xe2));
        palette.values.push_back(cv::Scalar(0xe5, 0xca, 0x57));
        palette.values.push_back(cv::Scalar(0x38, 0x61, 0xd9));
        palette.values.push_back(cv::Scalar(0xc6, 0x86, 0x3b));
        palette.values.push_back(cv::Scalar(0x57, 0xbf, 0xd7));
        palette.values.push_back(cv::Scalar(0xbb, 0x6b, 0x7c));
        palette.values.push_back(cv::Scalar(0x29, 0x92, 0xa5));
        palette.values.push_back(cv::Scalar(0xe2, 0x8a, 0xdc));
        palette.values.push_back(cv::Scalar(0x30, 0x8c, 0x6f));
        palette.values.push_back(cv::Scalar(0xc3, 0x78, 0x54));
        palette.values.push_back(cv::Scalar(0x24, 0x7e, 0xb5));
        palette.values.push_back(cv::Scalar(0xed, 0xb9, 0x74));
        palette.values.push_back(cv::Scalar(0x2c, 0x67, 0xb3));
        palette.values.push_back(cv::Scalar(0xda, 0x9f, 0x7d));
        palette.values.push_back(cv::Scalar(0x28, 0x78, 0x83));
        palette.values.push_back(cv::Scalar(0xf1, 0xaf, 0xb1));
        palette.values.push_back(cv::Scalar(0x7c, 0xcd, 0xab));
        palette.values.push_back(cv::Scalar(0x9d, 0x61, 0xbe));
        palette.values.push_back(cv::Scalar(0xa2, 0xd0, 0x8a));
        palette.values.push_back(cv::Scalar(0x99, 0x77, 0xe6));
        palette.values.push_back(cv::Scalar(0x80, 0x97, 0x39));
        palette.values.push_back(cv::Scalar(0x5c, 0x62, 0xd5));
        palette.values.push_back(cv::Scalar(0xb9, 0x9a, 0x36));
        palette.values.push_back(cv::Scalar(0x86, 0xc7, 0xe0));
        palette.values.push_back(cv::Scalar(0x9b, 0x63, 0x86));
        palette.values.push_back(cv::Scalar(0x54, 0x8a, 0x61));
        palette.values.push_back(cv::Scalar(0xd7, 0xa7, 0xe7));
        palette.values.push_back(cv::Scalar(0x46, 0x7c, 0x82));
        palette.values.push_back(cv::Scalar(0xc1, 0x86, 0xac));
        palette.values.push_back(cv::Scalar(0x68, 0xa4, 0xba));
        palette.values.push_back(cv::Scalar(0xaa, 0x74, 0x6a));
        palette.values.push_back(cv::Scalar(0x41, 0x76, 0x9f));
        palette.values.push_back(cv::Scalar(0xaa, 0x7c, 0x41));
        palette.values.push_back(cv::Scalar(0x87, 0x9b, 0xe7));
        palette.values.push_back(cv::Scalar(0x7b, 0x64, 0xb2));
        palette.values.push_back(cv::Scalar(0x54, 0x6c, 0xb1));
        return palette;
    }
};

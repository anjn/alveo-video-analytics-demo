#pragma once
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "msgpack.hpp"
#pragma GCC diagnostic pop

struct bcc_request_message
{
  int image_w;
  int image_h;
  int image_c;
  int image_stride;
  std::vector<uint8_t> image;

  MSGPACK_DEFINE(image_w, image_h, image_c, image_stride, image);
};

struct bcc_result_message
{
  int count;
  int map_w;
  int map_h;
  std::vector<uint8_t> map;

  MSGPACK_DEFINE(count, map_w, map_h, map);
};


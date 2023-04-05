#pragma once
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include "ml/base/inf_runner.hpp"
#include "ml/base/inf_server.hpp"
#include "ml/utils/vart_utils.hpp"
#include "ml/bcc/bcc_message.hpp"

struct bcc_model
{
    using request_t = bcc_request_message;
    using result_t = bcc_result_message;

    std::unique_ptr<vart::Runner> vart_runner;
    const inf_runner runner;
    const std::shared_ptr<host_buffer> in, out;
    const shape in_shape, out_shape;
    const int batch;

    const float mean[3] = { 103.53f, 116.28f, 123.675f };
    const float scale[3] = { 0.017429, 0.017507, 0.017125 };

    bcc_model(const std::vector<std::string>& xmodels) :
        vart_runner(create_vart_runner(xmodels[0])),
        runner(vart_runner.get()),
        in(runner["ResNet_BCC__input_0_fix"]),
        out(runner["ResNet_BCC__ResNet_BCC_Sequential_reg_layer__Conv2d_4__554_fix"]),
        in_shape(in->tensor->get_shape()),
        out_shape(out->tensor->get_shape()),
        batch(in_shape.batch)
    {
        auto tensor_format = vart_runner->get_tensor_format();
        if (tensor_format != vart::Runner::TensorFormat::NHWC)
            throw std::runtime_error("Unsupported tensor format!");
    }

    std::vector<result_t> run(const std::vector<request_t>& reqs)
    {
        const int batch = reqs.size();

        // Clear
        in->clear();

        // Preprocess
        std::vector<int> round_w(batch), round_h(batch), offset_x(batch), offset_y(batch);

        for (int i = 0; i < batch; i++)
        {
            auto& r = reqs[i];

            const int w = r.image_w;
            const int h = r.image_h;
            const int n = r.image_c;
            const int s = r.image_stride;
            assert(w <= in_shape.width);
            assert(h <= in_shape.height);

            round_w[i] = w / 8 * 8;
            round_h[i] = h / 8 * 8;
            offset_x[i] = (in_shape.width - round_w[i]) / 2 / 8 * 8;
            offset_y[i] = (in_shape.height - round_h[i]) / 2 / 8 * 8;

            //std::cout << "image : " << w << ", " << h << ", " << n << ", " << s << std::endl;
            //std::cout << "round : " << round_w[i] << ", " << round_h[i] << std::endl;
            //std::cout << "offset : " << offset_x[i] << ", " << offset_y[i] << std::endl;

            // Vitis AI image is in RGB format
            // U30 decoded image is in ABGR format?

            int8_t* ibp = in->pointer<int8_t>();
            const int batch_offset = in_shape.height * in_shape.width * in_shape.channel * i;
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    for (int c = 0; c < 3; c++) {
                        ibp[batch_offset + ((y + offset_y[i]) * in_shape.width + (x + offset_x[i])) * 3 + c] =
                            //int(roundf((r.image[y * s + x * n + (2 - c)] - mean[c]) * in->scale * scale[c])); // BGRA
                            int(roundf((r.image[y * s + x * n + (3 - c)] - mean[c]) * in->scale * scale[c])); // ABGR?
                    }
                }
            }

            //cv::Mat img(800, 1000, CV_8UC3, ibp);
            //cv::namedWindow("title", cv::WINDOW_NORMAL);
            //cv::imshow("title", img);
            //cv::waitKey();
        }

        // Infer
        runner.execute();

        // Postprocess
        std::vector<result_t> rsps(batch);

        for (int i = 0; i < batch; i++)
        {
            auto& r = rsps[i];
            r.map_w = round_w[i] / 8;
            r.map_h = round_h[i] / 8;
            r.map.resize(r.map_w * r.map_h);

            r.count = 0;
            int8_t* obp = out->pointer<int8_t>();
            const int batch_offset = out_shape.height * out_shape.width * out_shape.channel * i;
            for (int y = 0; y < out_shape.height; y++) {
                for (int x = 0; x < out_shape.width; x++) {
                    uint8_t o = std::max(int8_t(0), obp[batch_offset + y * out_shape.width + x]);
                    r.count += o;

                    if (offset_x[i] / 8 <= x && x < offset_x[i] / 8 + r.map_w &&
                        offset_y[i] / 8 <= y && y < offset_y[i] / 8 + r.map_h)
                    {
                        r.map[(y - offset_y[i] / 8) * r.map_w + (x - offset_x[i] / 8)] = std::min(255, int(o));
                    }
                }
            }
            r.count /= out->scale;

            //std::cout << "count : " << r.count << std::endl;
        }

        return rsps;
    }
};

using bcc_server = inf_server<bcc_model>;

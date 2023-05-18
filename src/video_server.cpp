#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <any>

#include <toml.hpp>
#include <arg/arg.h>

#include "video/rtsp_server.hpp"
#include "video/gst_app_bgr_utils.hpp"
#include "video/gst_pipeline_utils.hpp"
#include "utils/queue_mt.hpp"
#include "utils/time_utils.hpp"

#include "yolo.hpp"
#include "bcc.hpp"

using namespace std::chrono_literals;

struct compositor_bgr
{
    using this_type = compositor_bgr;
    using queue_ptr_t = app2queue_bgr::queue_ptr_t;

    std::vector<queue_ptr_t> queues;
    GstAppSrc* appsrc;

    int width;
    int height;

    cv::Mat mat;
    int gst_buffer_size;
    GstBuffer* last_buffer = nullptr;
    GstClockTime timestamp = 0;
    int fps_n = 15;
    int fps_d = 1;
    std::chrono::high_resolution_clock::time_point time;
    int focus_size = 0;
    int focus_duration = 8;

    compositor_bgr(
        std::vector<queue_ptr_t>& queues_,
        GstAppSrc* appsrc_,
        int width_,
        int height_
    ) :
        queues(queues_), appsrc(appsrc_), width(width_), height(height_)
    {
        GstVideoInfo info;
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGR, width, height);
        gst_buffer_size = info.size;
        assert(gst_buffer_size == width * height * 3);
        last_buffer = gst_buffer_new_allocate(nullptr, gst_buffer_size, nullptr);
        mat = cv::Mat(height, width, CV_8UC3);

        time = std::chrono::high_resolution_clock::now();
    }

    void start()
    {
        g_signal_connect(appsrc, "need-data", G_CALLBACK(&this_type::need_data), this);

        GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                            "format", G_TYPE_STRING, "BGR",
                                            "width",  G_TYPE_INT, width,
                                            "height", G_TYPE_INT, height,
                                            "framerate", GST_TYPE_FRACTION, fps_n, fps_d,
                                            nullptr);
        gst_app_src_set_caps(appsrc, caps);
        gst_caps_unref(caps);
    }

    static void need_data(GstElement *source, guint size, this_type* obj)
    {
        obj->push_buffer();
    }

    void push_buffer()
    {
        bool update = false;

        auto cur_time = std::chrono::high_resolution_clock::now();
        int elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(cur_time - time).count();
        int focus_camera = (elapsed_sec / focus_duration) % queues.size();

        int num_cameras = queues.size() + focus_size * focus_size;
        int num_windows = (int) std::ceil(std::sqrt(num_cameras));
        int resize_width = width / num_windows;
        int resize_height = height / num_windows;

        for (int i = 0; i < queues.size(); i++)
        {
            if (queues[i]->size() > 0)
            {
                auto buf = queues[i]->pop();

                // Draw focus window
                if (i == focus_camera && focus_size > 1) {
                    auto dst = cv::Mat(resize_height * focus_size, resize_width * focus_size, CV_8UC3);
                    cv::resize(buf, dst, cv::Size(dst.cols, dst.rows), 0, 0, cv::INTER_LINEAR);
                    cv::Rect roi(0, 0, resize_width * focus_size, resize_height * focus_size);
                    dst.copyTo(mat(roi));

                    // Draw rect
                    cv::rectangle(buf, cv::Point(0, 0), cv::Point(buf.cols, buf.rows), cv::Scalar(60, 240, 60), 20);
                }

                // Draw small window
                {
                    auto dst = cv::Mat(resize_height, resize_width, CV_8UC3);
                    cv::resize(buf, dst, cv::Size(dst.cols, dst.rows), 0, 0, cv::INTER_NEAREST);

                    int x, y;
                    if (i < (num_windows - focus_size) * focus_size) {
                        x = i % (num_windows - focus_size) + focus_size;
                        y = i / (num_windows - focus_size);
                    } else {
                        x = (i - (num_windows - focus_size) * focus_size) % num_windows;
                        y = (i - (num_windows - focus_size) * focus_size) / num_windows + focus_size;
                    }

                    cv::Rect roi(resize_width * x, resize_height * y, resize_width, resize_height);
                    dst.copyTo(mat(roi));
                }

                update = true;
            }
        }

        GstBuffer* buffer = nullptr;

        if (update)
        {
            buffer = gst_buffer_new_allocate(nullptr, gst_buffer_size, nullptr);

            // Show fps
            auto color = cv::Scalar(0x35, 0xf5, 0x3a);
            cv::putText(mat, "YOLOv3 : " + std::to_string(int(yolov3_fps.get())), cv::Point(10, 60), cv::FONT_HERSHEY_DUPLEX, 2.0, cv::Scalar(0,0,0), 7);
            cv::putText(mat, "YOLOv3 : " + std::to_string(int(yolov3_fps.get())), cv::Point(10, 60), cv::FONT_HERSHEY_DUPLEX, 2.0, color, 2);
            //cv::putText(mat, "YOLOv6m : " + std::to_string(int(yolov6_fps.get())), cv::Point(10, 60), cv::FONT_HERSHEY_DUPLEX, 2.0, cv::Scalar(0,0,0), 7);
            //cv::putText(mat, "YOLOv6m : " + std::to_string(int(yolov6_fps.get())), cv::Point(10, 60), cv::FONT_HERSHEY_DUPLEX, 2.0, color, 2);
            cv::putText(mat, "ResNet18 : " + std::to_string(int(car_fps.get() * 3)), cv::Point(1920 / 5 * 3 / 2, 60), cv::FONT_HERSHEY_DUPLEX, 2.0, cv::Scalar(0,0,0), 7);
            cv::putText(mat, "ResNet18 : " + std::to_string(int(car_fps.get() * 3)), cv::Point(1920 / 5 * 3 / 2, 60), cv::FONT_HERSHEY_DUPLEX, 2.0, color, 2);

            //g_print("output %d\n", gst_buffer_size);
            GstMapInfo info;
            gst_buffer_map(buffer, &info, GST_MAP_READ);
            std::memcpy(info.data, mat.data, width * height * 3);
            gst_buffer_unmap(buffer, &info);

            if (last_buffer) gst_buffer_unref(last_buffer);
            last_buffer = gst_buffer_ref(buffer);
        }
        else
        {
            buffer = gst_buffer_copy(last_buffer);
        }

        // Set timestamp
        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(fps_d, GST_SECOND, fps_n);
        timestamp += GST_BUFFER_DURATION(buffer);

        gst_app_src_push_buffer(appsrc, buffer);
    }
};

struct rtsp_ml_base
{
    const std::vector<cv::Size> sizes;
    
    rtsp_receiver receiver;
    vvas_dec dec;
    //vvas_scaler scaler;
    sw_multi_scaler scaler;
    std::vector<appsink> sinks;

    app2queue_bgr::queue_ptr_t queue;

    rtsp_ml_base(const std::string& location, int device_index, std::vector<cv::Size> sizes_) :
        sizes(sizes_),
        receiver(location),
        dec(device_index),
        //scaler(device_index),
        scaler(),
        sinks(sizes.size()),
        queue(std::make_shared<app2queue_bgr::queue_t>())
    {
        //scaler.multi_scaler = true;
    }

    std::string get_description()
    {
        std::string desc = build_pipeline_description(receiver, dec, scaler);

        for (int i = 0; i < sizes.size(); i++)
        {
            //desc += " " + scaler.scaler_name + ".src_" + std::to_string(i);
            desc += " " + scaler.get_source_pad(i);

            //if (!hw_config::get_instance()->support_scaler_bgr_output())
            //{
            //    desc += " ! video/x-raw, width=" + std::to_string(sizes[i].width) + ", height=" + std::to_string(sizes[i].height) + ", format=NV12";
            //    desc += " ! videoconvert";
            //}
            // TODO
            desc += " ! queue";
            desc += " ! videoscale";
            desc += " ! videoconvert";

            desc += " ! video/x-raw, width=" + std::to_string(sizes[i].width) + ", height=" + std::to_string(sizes[i].height) + ", format=BGR";

            desc += " ! queue ! ";
            desc += sinks[i].get_description();
        }

        return desc;
    }

    void set_params(GstElement* pipeline)
    {
        ::set_params(pipeline, receiver, dec, scaler);
        for (auto& sink : sinks) ::set_params(pipeline, sink);
    }

    virtual void start()
    {
        build_pipeline_and_play(*this);
    }
};

template<typename A2Q>
struct rtsp_ml : public rtsp_ml_base
{
    using parent_t = rtsp_ml_base;
    using parent_t::parent_t;

    std::shared_ptr<A2Q> a2q;

    void start() override
    {
        parent_t::start();

        std::vector<std::tuple<GstAppSink**, cv::Size>> appsinks;
        for (int i = 0; i < parent_t::sizes.size(); i++)
        {
            appsinks.push_back(std::make_tuple(&parent_t::sinks[i].sink, parent_t::sizes[i]));
        }

        a2q = std::make_shared<A2Q>(appsinks, parent_t::queue);
        a2q->start();
    }
};

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_i(num_devices, 1, "Num available devices");
    arg_end;

    // HW config
    hw_config::get_instance()->set_num_devices(num_devices);

    // Load config
    const auto config = toml::parse("config.toml");

    int output_rtsp      = toml::find<int>(config, "video", "output-rtsp");
    int output_width     = toml::find<int>(config, "video", "output-width");
    int output_height    = toml::find<int>(config, "video", "output-height");
    int output_bitrate   = toml::find<int>(config, "video", "output-bitrate");
    int focus_size       = toml::find<int>(config, "video", "focus-size");
    int focus_duration   = toml::find<int>(config, "video", "focus-duration");

    auto rtsp_server_location = toml::find<std::string>(config, "video", "rtsp-server-location");

    std::string ml_server_ip = "127.0.0.1";
    if (const char* e = std::getenv("ML_SERVER_IP")) ml_server_ip = e;

    auto set_address = [&](std::string name, std::string& address) {
        if (toml::find(config, "ml").contains(name))
            address = "tcp://" + ml_server_ip + ":" + std::to_string(toml::find<int>(config, "ml", name, "port"));
    };

    set_address("bcc", bcc_server_address);
    set_address("yolov3", yolov3_server_address);
    set_address("yolov6", yolov6_server_address);
    set_address("carclassification", car_server_address);

    std::string host_server_ip = "127.0.0.1";
    if (const char* e = std::getenv("HOST_SERVER_IP")) host_server_ip = e;
    influxdb_cpp::server_info server_info(host_server_ip, 8086, "org", "token", "bucket");

    gst_init(&argc, &argv);

    int device_index = 0;

    std::vector<app2queue_bgr::queue_ptr_t> queues;
    std::vector<std::shared_ptr<rtsp_ml_base>> ml_pipelines;

    std::vector<cv::Size> bcc_sizes;
    bcc_sizes.push_back(cv::Size(960, 540));

    std::vector<cv::Size> yolov3_sizes;
    yolov3_sizes.push_back(cv::Size(1152, 648));  // for display, 1920*3/5 = 1152
    yolov3_sizes.push_back(cv::Size(1920, 1080)); // for crop
    yolov3_sizes.push_back(cv::Size(416, 234));   // for inference

    std::vector<cv::Size> yolov6_sizes;
    yolov6_sizes.push_back(cv::Size(1152, 648));  // for display, 1920*3/5 = 1152
    yolov6_sizes.push_back(cv::Size(1920, 1080)); // for crop
    yolov6_sizes.push_back(cv::Size(640, 360));   // for inference

    for (auto& t : toml::find<std::vector<toml::table>>(config, "video", "cameras"))
    {
        auto id = toml::get<std::string>(t.at("id"));
        auto loc = toml::get<std::string>(t.at("location"));
        auto model = toml::get<std::string>(t.at("model"));

        std::cout << loc << ", " << model << std::endl;

        auto& ml = ml_pipelines.emplace_back();

        if (model == "bcc")
        {
            auto bcc = std::make_shared<rtsp_ml<app2queue_bcc>>(loc, device_index++, bcc_sizes);
            bcc->start();

            if (server_info.host_found)
                bcc->a2q->metrics_client = std::make_shared<metrics_client_influx_db>(id, server_info);

            ml = bcc;
        }
        else if (model == "yolov3" || model == "yolov6")
        {
            std::vector<cv::Size>* sizes = model == "yolov3" ? &yolov3_sizes : &yolov6_sizes;
            auto yolo = std::make_shared<rtsp_ml<app2queue_yolo>>(loc, device_index++, *sizes);
            yolo->start();

            yolo->a2q->set_yolo_version(model);

            std::vector<int> labels = toml::get<std::vector<int>>(t.at("labels"));
            yolo->a2q->set_detect_label_ids(labels);

            if (server_info.host_found)
                yolo->a2q->metrics_client = std::make_shared<metrics_client_influx_db>(id, server_info);

            ml = yolo;
        }

        queues.push_back(ml->queue);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Output pipeline
    appsrc src;

    videoconvert conv;
    rawvideomedia raw;
    raw.format = "NV12";

    hw_enc enc(device_index);
    enc.set_bitrate(output_bitrate);

    std::cout << rtsp_server_location << std::endl;
    rtspclientsink sink(rtsp_server_location);

    autovideosink sink1;

    if (output_rtsp)
        build_pipeline_and_play(src, conv, raw, enc, sink);
    else
        build_pipeline_and_play(src, sink1);

    // Compositor
    compositor_bgr comp(queues, src.src, output_width, output_height);
    comp.focus_size = focus_size;
    comp.focus_duration = focus_duration;
    comp.start();

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}

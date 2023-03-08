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
#include <ByteTrack/BYTETracker.h>

#include "video/rtsp_server.hpp"
#include "video/gst_app_utils.hpp"
#include "video/gst_pipeline_utils.hpp"
#include "video/hw_config_u30.hpp"
#include "ml/bcc/bcc_client.hpp"
#include "ml/yolov3/yolov3_client.hpp"
#include "utils/queue_mt.hpp"
#include "utils/time_utils.hpp"

using namespace std::chrono_literals;

std::string bcc_server_address = "tcp://127.0.0.1:5556";
std::string yolov3_server_address = "tcp://127.0.0.1:5555";

struct app2queue_bcc : app2queue
{
    bcc_client::queue_t result_queue;
    bcc_client client;
    cv::Mat heatmap;
    int count;

    app2queue_bcc(
        GstAppSink*& appsink_,
        queue_ptr_t& queue_,
        int width,
        int height
    ) :
        app2queue(appsink_, queue_, width, height),
        result_queue(bcc_client::create_result_queue()),
        client(bcc_server_address, result_queue),
        count(0)
    {}

    virtual void proc_buffer(cv::Mat& mat) override
    {
        // Request ML inference if client is not busy
        if (!client.is_busy())
        {
            client.request(mat.clone());
        }

        // Get ML inference result from queue
        while (result_queue->size() > 0)
        {
            std::tie(heatmap, count) = result_queue->pop();
        }

        // Draw heatmap
        if (!heatmap.empty())
        {
            for (int y = 0; y < mat.rows; y++) {
                for (int x = 0; x < mat.cols; x++) {
                    auto& p = mat.at<cv::Vec4b>(y, x);
                    auto& ph = heatmap.at<cv::Vec4b>(y / 8 , x / 8);
                    float pa = ph[3] / 255.0;
                    for (int c = 0; c < 3; c++) {
                        p[c] = p[c] * (1 - pa) + ph[c] * pa;
                    }
                }
            }
        }

        // Draw text
        cv::putText(mat, std::to_string(int(count)), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(0, 0, 0), 14);
        cv::putText(mat, std::to_string(int(count)), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(60, 240, 60), 6);
    }
};

struct app2queue_yolov3 : app2queue
{
    struct mytracker
    {
        std::vector<int> target_label_ids;
        byte_track::BYTETracker tracker;
        int frame = 0;
        int count = 0;
        std::unordered_map<size_t, int> track_id_to_count;
        std::unordered_map<size_t, size_t> track_id_to_last_detected;
    
        mytracker(const std::vector<int>& ids): target_label_ids(ids), tracker(10) {}
    };
    
    struct detection_result
    {
        byte_track::Rect<float> rect;
        int score;
        int label_id;
        size_t track_id;
        size_t frame_id;
        int count;
    };

    template<typename T>
    static byte_track::Rect<T> scale(const byte_track::Rect<T>& r, T value)
    {
        return { r.x() * value, r.y() * value, r.width() * value, r.height() * value };
    };

    yolov3_client::queue_t result_queue;
    yolov3_client client;

    std::vector<mytracker> trackers;

    std::vector<cv::Scalar> color_palette;

    std::vector<detection_result> detections;

    app2queue_yolov3(
        GstAppSink*& appsink_,
        queue_ptr_t& queue_,
        int width,
        int height
    ) :
        app2queue(appsink_, queue_, width, height),
        result_queue(yolov3_client::create_result_queue()),
        client(yolov3_server_address, result_queue)
    {
        // BGR order
        color_palette.push_back(cv::Scalar(0xcb, 0x5d, 0xf5));
        color_palette.push_back(cv::Scalar(0xff, 0x86, 0x63));
        color_palette.push_back(cv::Scalar(0x88, 0xe6, 0x49));
        color_palette.push_back(cv::Scalar(0x38, 0xce, 0xdc));
        color_palette.push_back(cv::Scalar(0x49, 0x82, 0xf5));
        color_palette.push_back(cv::Scalar(0x2f, 0x4a, 0xf0));
        color_palette.push_back(cv::Scalar(0x9c, 0x33, 0xf0));
        color_palette.push_back(cv::Scalar(0xf0, 0x2a, 0x34));
        color_palette.push_back(cv::Scalar(0xcf, 0xb0, 0xb0));
    }

    // 1,  bicycle
    // 5,  bus
    // 6,  car
    // 13, motorbike
    // 14, person
    void set_detect_label_ids(std::vector<int> label_ids)
    {
        trackers.emplace_back(label_ids);
    }

    virtual void proc_buffer(cv::Mat& mat) override
    {
        //std::cout << "app2queue_yolov3::proc_buffer" << std::endl;

        // Request ML inference if client is not busy
        if (!client.is_busy())
        {
            cv::Mat copy(mat.rows, mat.cols, CV_8UC3);
            cv::cvtColor(mat, copy, cv::COLOR_BGRA2BGR);
            client.request(copy);
        }

        // Get ML inference result from queue
        while (result_queue->size() > 0)
        {
            auto result = result_queue->pop();

            //std::cout << "detections " << result.detections.size() << std::endl;

            detections.clear();

            const float object_size_threshold = 0.00;
            const float size_threshold = object_size_threshold * object_size_threshold;

            // Tracking
            for (auto& tracker : trackers)
            {
                std::vector<byte_track::Object> objects;

                for (auto& b : result.detections)
                {
                    bool is_target = false;
                    for (auto& id : tracker.target_label_ids) if (b.label == id) is_target = true;

                    if (is_target && b.width * b.height > size_threshold)
                    {
                        byte_track::Rect rect { b.x, b.y, b.width, b.height };
                        rect = scale(rect, 1000.0f);
                        byte_track::Object obj { rect, b.label, b.prob };
                        objects.push_back(obj);
                    }
                }

                auto outputs = tracker.tracker.update(objects);

                for (auto& o : outputs)
                {
                    detection_result det;
                    det.rect = scale(o->getRect(), 0.001f);
                    det.score = o->getScore() * 100;
                    det.track_id = o->getTrackId();
                    det.frame_id = o->getFrameId();
                    det.label_id = tracker.target_label_ids[0];

                    if (!tracker.track_id_to_count.contains(det.track_id))
                    {
                        tracker.track_id_to_count[det.track_id] = ++tracker.count;
                    }
                    det.count = tracker.track_id_to_count[det.track_id];

                    tracker.track_id_to_last_detected[det.track_id] = det.frame_id;

                    detections.push_back(det);
                }

                // TODO Remove old track_id
            }
        }

        for (auto& det : detections)
        {
            auto& r0 = det.rect;

            byte_track::Rect r {
                int(r0.x() * mat.cols), int(r0.y() * mat.rows),
                int(r0.width() * mat.cols), int(r0.height() * mat.rows)
            };

            cv::Scalar color;
            std::string label;

            color = color_palette[det.count % color_palette.size()];

            double fs = 0.8;
            int tx = r.x() + 5;
            int th = 30 * fs;
            int ty = r.y() + 14 - th;
            int thickness = 2;

            cv::rectangle(mat,
                          cv::Point(r.x(), r.y()),
                          cv::Point(r.x() + r.width(), r.y() + r.height()),
                          color, 2, 1, 0);

            int baseline = 0;
            cv::Size text = cv::getTextSize(std::to_string(det.count), cv::FONT_HERSHEY_DUPLEX, fs, thickness, &baseline);
            cv::rectangle(mat, cv::Point(r.x(), ty + 3) + cv::Point(0, baseline), cv::Point(tx, ty) + cv::Point(text.width + 3, -text.height - 3), color, cv::FILLED, 1);
            cv::putText(mat, std::to_string(det.count), cv::Point(tx, ty + 2), cv::FONT_HERSHEY_DUPLEX, fs, cv::Scalar(240, 240, 240), thickness);
            ty += th + 6;
        }

        // Draw text
        cv::putText(mat, std::to_string(detections.size()), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(0, 0, 0), 14);
        cv::putText(mat, std::to_string(detections.size()), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(60, 240, 60), 6);
    }
};

struct compositor
{
    using this_type = compositor;

    std::vector<app2queue::queue_ptr_t> queues;
    GstAppSrc* appsrc;

    int width;
    int height;

    SwsContext* sws_ctx = nullptr;
    cv_buffer cv_buf;
    GstBuffer* last_buffer = nullptr;
    std::thread thread;
    GstClockTime timestamp = 0;
    int fps_n = 15;
    int fps_d = 1;
    std::chrono::high_resolution_clock::time_point time;
    int focus_size = 3;
    int focus_duration = 8;

    compositor(
        std::vector<app2queue::queue_ptr_t>& queues_,
        GstAppSrc* appsrc_,
        int width_,
        int height_
    ) :
        queues(queues_), appsrc(appsrc_), width(width_), height(height_), cv_buf(width, height)
    {
        GstVideoInfo info;
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_NV12, width, height);
        last_buffer = gst_buffer_new_allocate(nullptr, info.size, nullptr);
        cv_buf.set_meta(last_buffer);

        time = std::chrono::high_resolution_clock::now();
    }

    void start()
    {
        g_signal_connect(appsrc, "need-data", G_CALLBACK(&this_type::need_data), this);

        GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                            "format", G_TYPE_STRING, "NV12",
                                            "width",  G_TYPE_INT, width,
                                            "height", G_TYPE_INT, height,
                                            "framerate", GST_TYPE_FRACTION, fps_n, fps_d,
                                            nullptr);
        gst_app_src_set_caps(appsrc, caps);
        gst_caps_unref(caps);
    }

    static void need_data(GstElement *source, guint size, this_type* obj)
    {
        //std::cout << "compositor: need_data" << std::endl;
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
                    auto dst = cv::Mat(resize_height * focus_size, resize_width * focus_size, CV_8UC4);
                    cv::resize(buf->mat, dst, cv::Size(dst.cols, dst.rows), 0, 0, cv::INTER_NEAREST);
                    cv::Rect roi(0, 0, resize_width * focus_size, resize_height * focus_size);
                    dst.copyTo(cv_buf.mat(roi));

                    // Draw rect
                    cv::rectangle(buf->mat, cv::Point(0, 0), cv::Point(buf->mat.cols, buf->mat.rows), cv::Scalar(60, 240, 60), 20);
                }

                // Draw small window
                {
                    auto dst = cv::Mat(resize_height, resize_width, CV_8UC4);
                    cv::resize(buf->mat, dst, cv::Size(dst.cols, dst.rows), 0, 0, cv::INTER_NEAREST);

                    int x, y;
                    if (i < (num_windows - focus_size) * focus_size) {
                        x = i % (num_windows - focus_size) + focus_size;
                        y = i / (num_windows - focus_size);
                    } else {
                        x = (i - (num_windows - focus_size) * focus_size) % num_windows;
                        y = (i - (num_windows - focus_size) * focus_size) / num_windows + focus_size;
                    }

                    cv::Rect roi(resize_width * x, resize_height * y, resize_width, resize_height);
                    dst.copyTo(cv_buf.mat(roi));
                }

                update = true;
            }
        }

        GstBuffer* buffer = nullptr;

        if (update)
        {
            buffer = gst_buffer_new_allocate(nullptr, cv_buf.gst_buffer_size, nullptr);

            if (!sws_ctx) sws_ctx = cv_buf.create_sws_context_abgr_to_nv12();
            cv_buf.convert_abgr_to_nv12(sws_ctx, buffer);

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

template<typename HwConfig>
struct rtsp_ml_base
{
    const int width;
    const int height;
    
    rtsp_receiver receiver;
    vvas_dec<HwConfig> dec;
    vvas_scaler<HwConfig> scaler;
    appsink sink;

    app2queue::queue_ptr_t queue;

    //std::shared_ptr<A2Q> a2q;

    GstElement* pipeline;

    rtsp_ml_base(const std::string& location, int device_index) :
        width(960),
        height(540),
        receiver(location),
        dec(device_index),
        scaler(device_index, width, height),
        queue(std::make_shared<app2queue::queue_t>())
    {}

    virtual void start()
    {
        pipeline = build_pipeline(receiver, dec, scaler, sink);
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        //a2q = std::make_shared<A2Q>(
        //    sink.sink, queue, width, height
        //);
        //a2q->start();
    }
};

template<typename A2Q, typename HwConfig>
struct rtsp_ml : rtsp_ml_base<HwConfig>
{
    using parent_t = rtsp_ml_base<HwConfig>;
    using parent_t::parent_t;

    std::shared_ptr<A2Q> a2q;

    void start() override
    {
        parent_t::start();

        a2q = std::make_shared<A2Q>(
            parent_t::sink.sink, parent_t::queue, parent_t::width, parent_t::height
        );
        a2q->start();
    }
};

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_i(max_devices, 2, "Num available devices");
    arg_end;

    // HW config
    hw_config_u30::max_devices = max_devices;

    // Load config
    const auto config = toml::parse("config.toml");

    int rtsp_server_port = toml::find<int>(config, "video", "rtsp-server-port");
    int output_width     = toml::find<int>(config, "video", "output-width");
    int output_height    = toml::find<int>(config, "video", "output-height");
    int output_bitrate   = toml::find<int>(config, "video", "output-bitrate");
    int focus_size       = toml::find<int>(config, "video", "focus-size");
    int focus_duration   = toml::find<int>(config, "video", "focus-duration");

    std::string ml_server_ip = "127.0.0.1";
    if (const char* e = std::getenv("ML_SERVER_IP")) ml_server_ip = e;
    bcc_server_address    = "tcp://" + ml_server_ip + ":" + std::to_string(toml::find<int>(config, "ml", "bcc", "port"));
    yolov3_server_address = "tcp://" + ml_server_ip + ":" + std::to_string(toml::find<int>(config, "ml", "yolov3", "port"));

    gst_init(&argc, &argv);

    int device_index = 0;

    std::vector<app2queue::queue_ptr_t> queues;
    std::vector<std::shared_ptr<rtsp_ml_base<hw_config_u30>>> ml_pipelines;

    for (auto& t : toml::find<std::vector<toml::table>>(config, "video", "cameras"))
    {
        auto loc = toml::get<std::string>(t.at("location"));
        auto model = toml::get<std::string>(t.at("model"));

        std::cout << loc << ", " << model << std::endl;

        auto& ml = ml_pipelines.emplace_back();

        if (model == "bcc")
        {
            ml = std::make_shared<rtsp_ml<app2queue_bcc, hw_config_u30>>(loc, device_index++);
            ml->start();
        }
        else if (model == "yolov3")
        {
            auto yolo = std::make_shared<rtsp_ml<app2queue_yolov3, hw_config_u30>>(loc, device_index++);
            yolo->start();

            std::vector<int> labels = toml::get<std::vector<int>>(t.at("labels"));
            yolo->a2q->set_detect_label_ids(labels);

            ml = yolo;
        }
        queues.push_back(ml->queue);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    rtsp_server server("rtspsink", rtsp_server_port, device_index);
    server.bitrate = output_bitrate;
    server.start();

    appsrc src {};
    intervideosink sink { "rtspsink" };

    auto pipeline = build_pipeline(src, sink);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    compositor comp(queues, src.src, output_width, output_height);
    comp.focus_size = focus_size;
    comp.focus_duration = focus_duration;
    comp.start();

    g_print ("stream ready at rtsp://127.0.0.1:%d/test\n", server.port);

    auto loop = g_main_loop_new(nullptr, false);
    g_main_loop_run(loop);
}

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
#include <influxdb.hpp>

#include "video/rtsp_server.hpp"
#include "video/gst_app_bgr_utils.hpp"
#include "video/gst_pipeline_utils.hpp"
#include "video/hw_config_v70.hpp"
#include "ml/bcc/bcc_client.hpp"
#include "ml/yolov3/yolov3_client.hpp"
#include "ml/carclassification/carclassification_client.hpp"
#include "utils/queue_mt.hpp"
#include "utils/time_utils.hpp"

using namespace std::chrono_literals;

std::string yolov3_server_address = "tcp://127.0.0.1:5555";
std::string car_server_address = "tcp://127.0.0.1:5556";
std::string bcc_server_address = "tcp://127.0.0.1:5557";

fps_counter yolov3_fps(1000);
fps_counter car_fps(8000);
fps_counter bcc_fps(1000);

struct metrics_client_influx_db
{
    std::string id;

    influxdb_cpp::server_info server_info;

    metrics_client_influx_db(
        std::string id_,
        influxdb_cpp::server_info server_info_
    ) :
        id(id_),
        server_info(server_info_)
    {}

    void push(int count)
    {
        influxdb_cpp::builder()
            .meas("count")
            .tag("camera", id)
            .field("value", count)
            .post_http(server_info);
    }

    std::string get_id() const
    {
        return id;
    }
};

struct app2queue_bcc : app2queue_bgr
{
    bcc_client::queue_t result_queue;
    bcc_client client;
    cv::Mat heatmap;
    int count;

    std::shared_ptr<metrics_client_influx_db> metrics_client;

    app2queue_bcc(
        std::vector<std::tuple<GstAppSink**, cv::Size>> appsinks_,
        queue_ptr_t& queue_
    ) :
        app2queue_bgr(appsinks_, queue_),
        result_queue(bcc_client::create_result_queue()),
        client(bcc_server_address, result_queue),
        count(0)
    {}

    virtual void proc_buffer(std::vector<cv::Mat>& mats) override
    {
        auto mat = mats[0];

        // Request ML inference if client is not busy
        if (!client.is_busy())
        {
            client.request(mat.clone());
        }

        // Get ML inference result from queue
        while (result_queue->size() > 0)
        {
            std::tie(heatmap, count) = result_queue->pop();

            if (metrics_client)
                metrics_client->push(count);
        }

        // Draw heatmap
        if (!heatmap.empty())
        {
            for (int y = 0; y < mat.rows - 8; y++) {
                for (int x = 0; x < mat.cols; x++) {
                    auto& p = mat.at<cv::Vec3b>(y, x);
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

        //if (metrics_client)
        //{
        //    auto id = metrics_client->get_id();
        //    cv::putText(mat, id, cv::Point(24, mat.rows - 36), cv::FONT_HERSHEY_DUPLEX, 2, cv::Scalar(0, 0, 0), 14);
        //    cv::putText(mat, id, cv::Point(24, mat.rows - 36), cv::FONT_HERSHEY_DUPLEX, 2, cv::Scalar(255, 255, 255), 6);
        //}
    }
};

struct carclassification_score
{
    int count = 0;
    float color_scores[15];
    float make_scores[39];
    float type_scores[7];

    carclassification_score()
    {
        std::fill(color_scores, color_scores + 15, 0);
        std::fill(make_scores, make_scores + 39, 0);
        std::fill(type_scores, type_scores + 7, 0);
    }

    size_t color_index() {
        return std::distance(color_scores, std::max_element(color_scores, color_scores + 15));
    }
    size_t make_index() {
        return std::distance(make_scores, std::max_element(make_scores, make_scores + 39));
    }
    size_t type_index() {
        return std::distance(type_scores, std::max_element(type_scores, type_scores + 7));
    }
};

struct carclassification_runner
{
    mutable std::mutex mutex_scores;
    std::unordered_map<size_t, carclassification_score> track_id_to_scores;

    queue_mt<std::tuple<cv::Mat, cv::Rect, size_t>> request_queue;

    std::vector<std::thread> workers;

    carclassification_runner()
    {
        for (int i = 0; i < 6; i++) {
            workers.emplace_back([this]() {
                auto result_queue = carclassifcation_client::create_result_queue();
                carclassifcation_client client(car_server_address, result_queue);

                while(true)
                {
                    auto [img, box, track_id] = request_queue.pop();
                    auto car = crop_resize_for_carclassification(img, box);
                    client.request(car);

                    auto result = result_queue->pop();

                    {
                        std::unique_lock<std::mutex> lk(mutex_scores);
                        auto& score = track_id_to_scores[track_id];
                        score.color_scores[result.color.label_id] += result.color.score;
                        score.make_scores[result.make.label_id] += result.make.score;
                        score.type_scores[result.type.label_id] += result.type.score;
                        score.count++;
                    }

                    car_fps.count();
                }
            });
        }
    }

    template<typename Func>
    void get_score(size_t track_id, Func func)
    {
        std::unique_lock<std::mutex> lk(mutex_scores);
        if (auto it = track_id_to_scores.find(track_id); it != track_id_to_scores.end()) {
            func(it->second);
        }
    }
};

struct app2queue_yolov3 : app2queue_bgr
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
    std::queue<T> buffer_queue;

    std::vector<mytracker> trackers;

    carclassification_runner car_runner;

    std::vector<cv::Scalar> color_palette;

    std::vector<detection_result> detections;

    std::shared_ptr<metrics_client_influx_db> metrics_client;

    app2queue_yolov3(
        std::vector<std::tuple<GstAppSink**, cv::Size>> appsinks_,
        queue_ptr_t& queue_
    ) :
        app2queue_bgr(appsinks_, queue_),
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

    virtual void proc_buffer(std::vector<cv::Mat>& mats) override
    {
        auto mat = mats[0];
        //std::cout << "app2queue_yolov3::proc_buffer" << std::endl;

        // Request ML inference if client is not busy
        if (!client.is_busy())
        {
            client.request(mat.clone());
        }

        // Get ML inference result from queue
        while (result_queue->size() > 0)
        {
            auto [result, img] = result_queue->pop();
            yolov3_fps.count();

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

                if (metrics_client)
                    metrics_client->push(detections.size());

                // TODO Remove old track_id
            }
            
            // Sort detections by classification count
            std::vector<std::pair<int, int>> det_index_to_count;
            for (int i = 0; i < detections.size(); i++)
            {
                // Run classification only for cars
                if (detections[i].label_id == 1) // TODO bicycle?
                {
                    int track_id = detections[i].track_id;
                    int count = 0;
                    car_runner.get_score(track_id, [&count](carclassification_score& score) { count = score.count; });
                    det_index_to_count.emplace_back(i, count);
                }
            }

            std::sort(det_index_to_count.begin(), det_index_to_count.end(), [](auto& a, auto& b) { return a.second < b.second; });

            int num_classification = std::min(4ul - car_runner.request_queue.size(), det_index_to_count.size());

            for (int i = 0; i < num_classification; i++)
            {
                auto& det = detections[det_index_to_count[i].first];
                cv::Rect rect {
                    int(det.rect.x() * img.cols), int(det.rect.y() * img.rows),
                    int(det.rect.width() * img.cols), int(det.rect.height() * img.rows)
                };
                car_runner.request_queue.push(std::make_tuple(img, rect, det.track_id));
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

            car_runner.get_score(det.track_id, [&](carclassification_score& score) {
                auto& color_label = vehicle_color_labels[score.color_index()];
                auto& make_label = vehicle_make_labels[score.make_index()];
                auto& type_label = vehicle_type_labels[score.type_index()];

                cv::putText(mat, color_label, cv::Point(tx, ty), cv::FONT_HERSHEY_DUPLEX, fs, color, thickness);
                ty += th;
                cv::putText(mat, make_label, cv::Point(tx, ty), cv::FONT_HERSHEY_DUPLEX, fs, color, thickness);
                ty += th;
                cv::putText(mat, type_label, cv::Point(tx, ty), cv::FONT_HERSHEY_DUPLEX, fs, color, thickness);
                ty += th;
            });
        }

        // Draw text
        //cv::putText(mat, std::to_string(detections.size()), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(0, 0, 0), 14);
        //cv::putText(mat, std::to_string(detections.size()), cv::Point(24, 100), cv::FONT_HERSHEY_DUPLEX, 3, cv::Scalar(60, 240, 60), 6);

        //if (metrics_client)
        //{
        //    auto id = metrics_client->get_id();
        //    cv::putText(mat, id, cv::Point(24, mat.rows - 36), cv::FONT_HERSHEY_DUPLEX, 2, cv::Scalar(0, 0, 0), 14);
        //    cv::putText(mat, id, cv::Point(24, mat.rows - 36), cv::FONT_HERSHEY_DUPLEX, 2, cv::Scalar(255, 255, 255), 6);
        //}
    }
};

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
    std::thread thread;
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
                auto buf = queues[i]->pop()[0];

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

template<typename HwConfig>
struct rtsp_ml_base
{
    const std::vector<cv::Size> sizes;
    
    rtsp_receiver receiver;
    vvas_dec<HwConfig> dec;
    vvas_scaler<HwConfig> scaler;
    std::vector<appsink> sinks;

    app2queue_bgr::queue_ptr_t queue;

    GstElement* pipeline;

    rtsp_ml_base(const std::string& location, int device_index, std::vector<cv::Size> sizes_) :
        sizes(sizes_),
        receiver(location),
        dec(device_index),
        scaler(device_index),
        sinks(sizes.size()),
        queue(std::make_shared<app2queue_bgr::queue_t>())
    {
        scaler.multi_scaler = true;
    }

    std::string get_description()
    {
        std::string desc = build_pipeline_description(receiver, dec, scaler);

        for (int i = 0; i < sizes.size(); i++)
        {
            desc += " " + scaler.scaler_name + ".src_" + std::to_string(i);
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

template<typename A2Q, typename HwConfig>
struct rtsp_ml : public rtsp_ml_base<HwConfig>
{
    using parent_t = rtsp_ml_base<HwConfig>;
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
    //arg_i(max_devices, 2, "Num available devices");
    arg_end;

    // HW config
    //hw_config_v70::max_devices = max_devices;

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
    bcc_server_address    = "tcp://" + ml_server_ip + ":" + std::to_string(toml::find<int>(config, "ml", "bcc", "port"));
    yolov3_server_address = "tcp://" + ml_server_ip + ":" + std::to_string(toml::find<int>(config, "ml", "yolov3", "port"));
    car_server_address = "tcp://" + ml_server_ip + ":" + std::to_string(toml::find<int>(config, "ml", "carclassification", "port"));

    std::string host_server_ip = "127.0.0.1";
    if (const char* e = std::getenv("HOST_SERVER_IP")) host_server_ip = e;
    influxdb_cpp::server_info server_info(host_server_ip, 8086, "org", "token", "bucket");

    gst_init(&argc, &argv);

    int device_index = 0;

    std::vector<app2queue_bgr::queue_ptr_t> queues;
    std::vector<std::shared_ptr<rtsp_ml_base<hw_config_v70>>> ml_pipelines;

    std::vector<cv::Size> bcc_sizes;
    bcc_sizes.push_back(cv::Size(960, 540));

    for (auto& t : toml::find<std::vector<toml::table>>(config, "video", "cameras"))
    {
        auto id = toml::get<std::string>(t.at("id"));
        auto loc = toml::get<std::string>(t.at("location"));
        auto model = toml::get<std::string>(t.at("model"));

        std::cout << loc << ", " << model << std::endl;

        auto& ml = ml_pipelines.emplace_back();

        if (model == "bcc")
        {
            auto bcc = std::make_shared<rtsp_ml<app2queue_bcc, hw_config_v70>>(loc, device_index++, bcc_sizes);
            bcc->start();

            if (server_info.host_found)
                bcc->a2q->metrics_client = std::make_shared<metrics_client_influx_db>(id, server_info);

            ml = bcc;
        }
        else if (model == "yolov3")
        {
            auto yolo = std::make_shared<rtsp_ml<app2queue_yolov3, hw_config_v70>>(loc, device_index++, bcc_sizes);
            yolo->start();

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

    //vvas_enc<hw_config_v70> enc(device_index);
    x264enc enc;
    enc.bitrate = output_bitrate;

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

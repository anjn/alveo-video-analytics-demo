#pragma once

#include "color_palette.hpp"
#include "ml/yolo_common/ms_coco.hpp"
#include "ml/yolo_common/yolo_client.hpp"
#include "ml/yolo_common/voc.hpp"
#include "utils/time_utils.hpp"
#include "video/gst_app_bgr_utils.hpp"

#include "carclassification.hpp"
#include "tracker.hpp"
#include "color_palette.hpp"
#include "metrics.hpp"

inline std::string yolov3_server_address;
inline fps_counter yolov3_fps(1000);

inline std::string yolov6_server_address;
inline fps_counter yolov6_fps(1000);

struct app2queue_yolo : app2queue_bgr
{
    yolo_client::queue_t result_queue;
    std::vector<std::shared_ptr<yolo_client>> clients;
    std::queue<std::vector<cv::Mat>> buffer_queue;

    std::vector<tracker> trackers;

    carclassification_runner car_runner;

    color_palette palette;

    std::vector<std::string> labels;
    std::string server_address;
    int car_label_id;
    fps_counter* fps;

    std::shared_ptr<metrics_client_influx_db> metrics_client;

    app2queue_yolo(
        std::vector<std::tuple<GstAppSink**, cv::Size>> appsinks_,
        queue_ptr_t& queue_
    ) :
        app2queue_bgr(appsinks_, queue_),
        result_queue(yolo_client::create_result_queue()),
        palette(color_palette::get_default()),
        fps(nullptr)
    {}

    void set_yolo_version(const std::string& model)
    {
        if (model == "yolov3")
        {
            labels = voc_labels;
            server_address = yolov3_server_address;
            fps = &yolov3_fps;
            car_label_id = 6;
        }
        else if (model == "yolov6")
        {
            labels = coco_labels;
            server_address = yolov6_server_address;
            fps = &yolov6_fps;
            car_label_id = 2;
        }

        clients.clear();
        for (int i = 0; i < 3; i++) {
            clients.push_back(std::make_shared<yolo_client>(server_address, result_queue));
        }
    }

    // 1,  bicycle
    // 5,  bus
    // 6,  car
    // 13, motorbike
    // 14, person
    void set_detect_label_ids(std::vector<int> label_ids)
    {
        for (auto label_id: label_ids) {
            trackers.emplace_back(std::vector { label_id });
        }
    }

    virtual void proc_buffer(std::vector<cv::Mat>& mats) override
    {
        // Request ML inference if clients are not busy
        for (auto& client: clients) {
            if (!client->is_busy()) {
                client->request(mats[2]); // 2 for infer
                buffer_queue.push(mats);
            }
        }

        // Get ML inference result from queue
        while (result_queue->size() > 0)
        {
            auto result = result_queue->pop();
            auto inferred_mats = buffer_queue.front();
            buffer_queue.pop();
            auto img = inferred_mats[1]; // 1 for crop

            if (fps) fps->count();

            std::vector<tracker::result> detected_cars;

            // Tracking
            for (auto& tracker : trackers)
            {
                tracker.update(result);

//                if (metrics_client)
//                    metrics_client->push(tracker.detections.size());
//
                for (auto& det: tracker.detections) {
                    if (det.label_id == car_label_id) detected_cars.push_back(det);
                }

                // TODO Remove old track_id to free memory
            }

            // Run car classification
            car_runner.request(img, detected_cars);
        }

        auto mat = mats[0]; // 0 for display

        // Draw result
        for (auto& tracker : trackers)
        {
            for (auto& det : tracker.detections)
            {
                auto& r0 = det.rect;
    
                byte_track::Rect r {
                    int(r0.x() * mat.cols), int(r0.y() * mat.rows),
                    int(r0.width() * mat.cols), int(r0.height() * mat.rows)
                };
    
                cv::Scalar color;
    
                color = palette[det.count];
    
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
                std::string label = labels[det.label_id] + ":" + std::to_string(det.count);
                cv::Size text = cv::getTextSize(label, cv::FONT_HERSHEY_DUPLEX, fs, thickness, &baseline);
                cv::rectangle(mat, cv::Point(r.x(), ty + 3) + cv::Point(0, baseline), cv::Point(tx, ty) + cv::Point(text.width + 3, -text.height - 3), color, cv::FILLED, 1);
                cv::putText(mat, label, cv::Point(tx, ty + 2), cv::FONT_HERSHEY_DUPLEX, fs, cv::Scalar(240, 240, 240), thickness);
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

#pragma once
#include <vector>

#include <ByteTrack/BYTETracker.h>

#include "ml/yolo_common/yolo_message.hpp"
#include "utils/time_utils.hpp"

struct tracker
{
    struct result
    {
        byte_track::Rect<float> rect;
        int score;
        int label_id;
        size_t track_id;
        size_t frame_id;
        int count;
    };

    std::vector<int> target_label_ids;
    byte_track::BYTETracker algo;
    int frame = 0;
    int count = 0;
    std::unordered_map<size_t, int> track_id_to_count;
    std::unordered_map<size_t, size_t> track_id_to_last_detected;

    std::vector<result> detections;

    tracker(const std::vector<int>& ids): target_label_ids(ids), algo(10) {}

    template<typename T>
    static byte_track::Rect<T> scale(const byte_track::Rect<T>& r, T value)
    {
        return { r.x() * value, r.y() * value, r.width() * value, r.height() * value };
    };

    void update(yolo_result_message res)
    {
        const float object_size_threshold = 0.00;
        const float size_threshold = object_size_threshold * object_size_threshold;

        std::vector<byte_track::Object> objects;

        for (auto& b : res.detections)
        {
            if (target(b.label) && b.width * b.height > size_threshold)
            {
                byte_track::Rect rect { b.x, b.y, b.width, b.height };
                rect = scale(rect, 1000.0f);
                byte_track::Object obj { rect, b.label, b.prob };
                objects.push_back(obj);
            }
        }

        //stop_watch sw;
        auto outputs = algo.update(objects);
        //sw.time("track:" + std::to_string(objects.size()));

        detections.clear();

        for (auto& o : outputs)
        {
            result det;
            det.rect = scale(o->getRect(), 0.001f);
            det.score = o->getScore() * 100;
            det.track_id = o->getTrackId();
            det.frame_id = o->getFrameId();
            det.label_id = target_label_ids[0];

            if (!track_id_to_count.contains(det.track_id))
            {
                track_id_to_count[det.track_id] = ++count;
            }
            det.count = track_id_to_count[det.track_id];

            track_id_to_last_detected[det.track_id] = det.frame_id;

            detections.push_back(det);
        }
    }

    bool target(int label)
    {
        for (auto& target_label : target_label_ids) if (label == target_label) return true;
        return false;
    }
};

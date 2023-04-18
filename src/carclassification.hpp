#pragma once

#include "ml/carclassification/carclassification_client.hpp"
#include "utils/time_utils.hpp"
#include "tracker.hpp"

inline std::string car_server_address;
inline fps_counter car_fps(8000);

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
        for (int i = 0; i < 3; i++) {
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

    void request(cv::Mat& img, const std::vector<tracker::result>& detections)
    {
        // Sort detections by classification count
        std::vector<std::pair<int, int>> det_index_to_count;
        {
            std::unique_lock<std::mutex> lk(mutex_scores);
            for (int i = 0; i < detections.size(); i++)
            {
                int track_id = detections[i].track_id;
                int count = 0;
                if (auto it = track_id_to_scores.find(track_id); it != track_id_to_scores.end()) {
                    count = it->second.count;
                }
                det_index_to_count.emplace_back(i, count);
            }
        }

        std::sort(det_index_to_count.begin(), det_index_to_count.end(),
                [](auto& a, auto& b) { return a.second < b.second; });

        int num_classification = std::min(2ul - request_queue.size(), det_index_to_count.size());

        for (int i = 0; i < num_classification; i++)
        {
            auto& det = detections[det_index_to_count[i].first];
            cv::Rect rect {
                int(det.rect.x() * img.cols), int(det.rect.y() * img.rows),
                    int(det.rect.width() * img.cols), int(det.rect.height() * img.rows)
            };
            request_queue.push(std::make_tuple(img, rect, det.track_id));
        }
    }
};


#pragma once
#include <array>

#include "ml/carclassification/carclassification_client.hpp"
#include "utils/time_utils.hpp"
#include "tracker.hpp"

inline std::string car_server_address;
inline fps_counter car_fps(8000);

struct carclassification_score
{
    static const int num_color = 15;
    static const int num_make = 39;
    static const int num_type = 7;

    int count = 0;
    std::array<float, num_color> color_scores;
    std::array<float, num_make > make_scores ;
    std::array<float, num_type > type_scores ;

    carclassification_score()
    {
        std::fill(std::begin(color_scores), std::end(color_scores), 0);
        std::fill(std::begin(make_scores ), std::end(make_scores ), 0);
        std::fill(std::begin(type_scores ), std::end(type_scores ), 0);
    }

    size_t color_index() {
        return std::distance(std::begin(color_scores), std::max_element(std::begin(color_scores), std::end(color_scores)));
    }
    size_t make_index() {
        return std::distance(std::begin(make_scores), std::max_element(std::begin(make_scores), std::end(make_scores)));
    }
    size_t type_index() {
        return std::distance(std::begin(type_scores), std::max_element(std::begin(type_scores), std::end(type_scores)));
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
                        // Forget old scores
                        for (auto& s: score.color_scores) s *= 0.9f;
                        for (auto& s: score.make_scores ) s *= 0.9f;
                        for (auto& s: score.type_scores ) s *= 0.9f;
                        // Add new scores
                        for (auto color: result.color) { if (color.label_id < score.color_scores.size()) score.color_scores[color.label_id] += color.score; }
                        for (auto make : result.make ) { if (make .label_id < score.make_scores .size()) score.make_scores [make .label_id] += make .score; }
                        for (auto type : result.type ) { if (type .label_id < score.type_scores .size()) score.type_scores [type .label_id] += type .score; }
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


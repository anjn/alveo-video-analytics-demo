#pragma once
#include <chrono>
#include <iostream>
#include <list>
#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <sstream>
#include <mutex>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic pop

#include <vitis/ai/yolov3.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "msgpack.hpp"
#pragma GCC diagnostic pop

#include <zmq.hpp>

#include "ml/base/inf_dynamic_batching.hpp"

struct inf_server_config
{
    std::string name;
    std::vector<std::string> xmodels;
    std::string address;
    int max_batch_latency;
    int max_batch_size;
    int num_workers;
};

template<typename Model>
class inf_server
{
private:
    const inf_server_config conf;

    // Worker threads
    std::vector<std::thread> threads;

    // Proxy thread
    std::thread proxy_thread;

    // Zmq
    zmq::context_t zmq_context;
    zmq::socket_t clients, workers;

public:

    using request_t = Model::request_t;
    using result_t = Model::result_t;

    inf_server(const inf_server_config& conf_) :
        conf(conf_),
        zmq_context(1),
        clients(zmq_context, ZMQ_ROUTER),
        workers(zmq_context, ZMQ_DEALER)
    {}

    void start(bool detach = false)
    {
        //std::cout << "'" << conf.name << "'" << std::endl;
        //std::cout << "'" << conf.address << "'" << std::endl;

        // Start server
        clients.bind(conf.address);
        workers.bind("inproc://" + conf.name + "-workers");

        // Start workers
        for (int i = 0; i < conf.num_workers; i++) {
            threads.emplace_back(&inf_server::worker_process, this);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Start proxy
        proxy_thread = std::thread([this]() {
            //zmq::proxy(clients, workers);
            dynamic_batching(clients, workers, conf.max_batch_size, conf.max_batch_latency);
        });

        std::cout << "Started " << conf.name << " server at " << conf.address <<
            " with " << std::to_string(conf.num_workers) << " workers " << std::endl;

        if (detach)
            proxy_thread.detach();
        else
            proxy_thread.join();
    }

private:

    void worker_process()
    {
        zmq::socket_t socket(zmq_context, ZMQ_REP);
        socket.connect("inproc://" + conf.name + "-workers");

        Model model { conf.xmodels };

        while (true)
        {
            // Receive batch
            std::vector<request_t> requests;

            while (true) {
                // Get request
                zmq::message_t req;
                int more;
                socket.recv(req);
                size_t more_size = sizeof(more);
                socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);

                // Deserialize
                auto oh = msgpack::unpack(static_cast<const char*>(req.data()), req.size());
                auto o = oh.get();
                o.convert(requests.emplace_back());

                if (!more) break;
            }

            const int batch_size = requests.size();

            // Run infer
            std::vector<result_t> results = model.run(requests);

            // Return results
            for (auto i = 0u; i < batch_size; i++)
            {
                // Serialize
                std::stringstream ss;
                msgpack::pack(ss, results[i]);

                // Send response
                zmq::message_t res(ss.str());
                socket.send(res, i + 1 < batch_size ? zmq::send_flags::sndmore : zmq::send_flags::none);
            }
        }
    }
};


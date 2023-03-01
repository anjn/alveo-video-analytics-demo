#pragma once
#include <thread>
#include <string>
#include <memory>
#include <sstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include "msgpack.hpp"
#pragma GCC diagnostic pop

#include <zmq.hpp>

#include "utils/queue_mt.hpp"

template<typename Adapter>
class inf_client
{
    using request_t = Adapter::request_t;
    using result_t = Adapter::result_t;
    using request_message_t = Adapter::request_message_t;
    using result_message_t = Adapter::result_message_t;

    zmq::context_t zmq_context;
    zmq::socket_t socket;
    std::atomic<bool> busy;
    std::thread worker;
    queue_mt<request_t> request_queue;
    std::shared_ptr<queue_mt<result_t>> result_queue;
    Adapter adapter;

public:

    using client_t = std::shared_ptr<inf_client<Adapter>>;
    using queue_t = std::shared_ptr<queue_mt<result_t>>;

    inf_client() {}

    inf_client(
        const std::string& server_address,
        std::shared_ptr<queue_mt<result_t>> result_queue_
    ) :
        zmq_context(1),
        socket(zmq_context, ZMQ_REQ),
        busy(false),
        result_queue(result_queue_)
    {
        socket.connect(server_address);

        worker = std::thread([this]() {
            while(true)
            {
                auto request = request_queue.pop();

                busy = true;

                // Pack request
                std::stringstream ss;
                msgpack::pack(ss, adapter.create_request(request));

                // Send request
                zmq::message_t req(ss.str());
                socket.send(req, zmq::send_flags::none);

                // Get response
                zmq::message_t res;
                socket.recv(res);

                // Deserialize
                auto oh = msgpack::unpack(static_cast<const char*>(res.data()), res.size());
                auto o = oh.get();
                result_message_t result;
                o.convert(result);

                result_queue->push(adapter.create_result(result));

                busy = false;
            }
        });
    }

    static auto create_client()
    {
        return std::make_shared<inf_client<Adapter>>();
    }

    static auto create_result_queue()
    {
        return std::make_shared<queue_mt<result_t>>();
    }

    bool is_busy()
    {
        return busy;
    }

    void request(const request_t& request)
    {
        request_queue.push(request);
    }
};



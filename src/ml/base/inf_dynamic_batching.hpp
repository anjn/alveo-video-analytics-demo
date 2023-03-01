#pragma once
#include <chrono>
#include <map>
#include <vector>

#include "zmq.hpp"

//namespace demo {

static void dynamic_batching(
  zmq::socket_t& frontend,
  zmq::socket_t& backend,
  int max_batch,
  int max_latency_ms
) {

  zmq::pollitem_t items[] = {
    { static_cast<void*>(frontend), 0, ZMQ_POLLIN, 0 },
    { static_cast<void*>(backend), 0, ZMQ_POLLIN, 0 }
  };

  uint64_t batch_id = 0;
  std::map<uint64_t, std::vector<zmq::message_t>> batches;
  std::vector<zmq::message_t> message_queue;
  std::chrono::system_clock::time_point batch_start;

  auto more = [](zmq::socket_t& s) {
    int v; size_t more_size = sizeof(v);
    s.getsockopt(ZMQ_RCVMORE, &v, &more_size);
    return v;
  };

  while (true) {
    zmq::poll(&items[0], 2, 1); // timeout = 1ms

    if (items[0].revents & ZMQ_POLLIN) {
      //std::cout << "Batch: from frontend, bi=" << batch_id << std::endl;

      zmq::message_t m;

      // Address
      frontend.recv(m);
      assert(more(frontend) != 0);
      batches[batch_id].emplace_back(m.data(), m.size());

      // Empty
      frontend.recv(m);
      assert(more(frontend) != 0);
      assert(m.size() == 0);

      // Body
      frontend.recv(m);
      assert(more(frontend) == 0);
      message_queue.emplace_back(m.data(), m.size());

      if (message_queue.size() == 1) {
        batch_start = std::chrono::system_clock::now();
      }
    }

    // Send to backend
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - batch_start).count();
    if (message_queue.size() >= max_batch ||
        (message_queue.size() > 0 && elapsed >= max_latency_ms))
    {
      //std::cout << "Batch: to backend, bi=" << batch_id << ", bs=" << message_queue.size() << std::endl;

      zmq::message_t m_id(&batch_id, sizeof(batch_id));
      backend.send(m_id, zmq::send_flags::sndmore);

      zmq::message_t m_empty;
      backend.send(m_empty, zmq::send_flags::sndmore);

      for (auto i=0u; i<message_queue.size(); i++) {
        backend.send(message_queue[i], i+1 < message_queue.size() ? zmq::send_flags::sndmore : zmq::send_flags::none);
      }

      message_queue.clear();
      batch_id++;
    }

    if (items[1].revents & ZMQ_POLLIN) {
      zmq::message_t m;

      // Batch ID
      backend.recv(m);
      assert(more(backend) != 0);
      assert(m.size() == sizeof(batch_id));
      uint64_t bi = *reinterpret_cast<uint64_t*>(m.data());
      assert(batches.find(bi) != batches.end());

      // Empty
      backend.recv(m);
      assert(more(backend) != 0);
      assert(m.size() == 0);

      //std::cout << "Batch: to frontend, bi=" << bi << ", bs=" << batches[bi].size() << ", " << batches.size() << std::endl;

      // Body
      for (auto& addr: batches[bi]) {
        // Address
        frontend.send(addr, zmq::send_flags::sndmore);

        // Empty
        zmq::message_t m_empty;
        frontend.send(m_empty, zmq::send_flags::sndmore);

        // Body
        backend.recv(m);
        frontend.send(m, zmq::send_flags::none);
      }

      batches.erase(bi);
    }
  }
}

//}

#pragma once
#include <iostream>
#include <vector>

#include <vart/runner.hpp>
#include <vart/runner_helper.hpp>
#include <xir/tensor/tensor.hpp>

struct host_buffer
{
    const xir::Tensor* const tensor;
    const std::string name;
    const std::vector<int32_t> shape;
    const std::unique_ptr<vart::TensorBuffer> buffer;
    void* const buffer_ptr;
    const float scale;

    host_buffer(const xir::Tensor* tensor_) : 
        tensor(tensor_),
        name(tensor->get_name()),
        shape(tensor->get_shape()),
        buffer(vart::alloc_cpu_flat_tensor_buffer(tensor)),
        buffer_ptr(reinterpret_cast<void*>(buffer->data().first)),
        scale(std::exp2f(tensor->template get_attr<int>("fix_point")))
    {
    }

    void clear() {
        memset(buffer_ptr, 0, tensor->get_data_size());
    }

    template<typename T>
    T* pointer() {
        return reinterpret_cast<T*>(buffer_ptr);
    }
};

struct inf_runner
{
    vart::Runner* const runner;
    std::map<std::string, std::shared_ptr<host_buffer>> host_buffers;
    std::vector<vart::TensorBuffer*> inputs, outputs;

    inf_runner(vart::Runner* runner_) :
        runner(runner_)
    {
        for (const auto& t: runner->get_input_tensors()) {
            auto b = std::make_shared<host_buffer>(t);
            host_buffers.emplace(std::make_pair(b->name, b));
            inputs.push_back(b->buffer.get());
        }
        for (const auto& t: runner->get_output_tensors()) {
            auto b = std::make_shared<host_buffer>(t);
            host_buffers.emplace(std::make_pair(b->name, b));
            outputs.push_back(b->buffer.get());
        }
    }

    auto operator[](const std::string& name) const
    {
        auto it = host_buffers.find(name);
        if (it == host_buffers.end()) throw std::runtime_error("Couldn't find buffer '" + name + "'!");
        return it->second;
    }

    void execute() const
    {
        auto job = runner->execute_async(inputs, outputs);
        runner->wait(job.first, -1);
    }
};

struct shape
{
    int batch;
    int height;
    int width;
    int channel;

    shape() {}

    shape(const std::vector<int>& s) :
        batch(s[0]),
        height(s[1]),
        width(s[2]),
        channel(s[3])
    {}
};


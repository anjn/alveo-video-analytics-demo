#pragma once
#include <vector>

#include <vart/runner.hpp>

#include "xir_utils.hpp"

static std::unique_ptr<vart::Runner> create_vart_runner(const std::string& xmodel)
{
    auto graph = xir::Graph::deserialize(xmodel);
    auto subgraphs = get_dpu_subgraphs(graph.get());
    if (subgraphs.size() != 1)
        throw std::runtime_error("Multiple DPU subgraphs is not supported!");
    return vart::Runner::create_runner(subgraphs[0], "run");
}

#pragma once
#include <vector>

#include <xir/graph/graph.hpp>

static std::vector<xir::Subgraph*> get_dpu_subgraphs(xir::Graph* graph)
{
    auto root = graph->get_root_subgraph();
    auto children = root->children_topological_sort();
    auto ret = std::vector<xir::Subgraph*>();
    for (auto c : children) {
        auto device = c->get_attr<std::string>("device");
        if (device == "DPU") {
            ret.emplace_back(c);
        }
    }
    return ret;
}

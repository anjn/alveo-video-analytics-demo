#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>

#include <toml.hpp>
#include <arg/arg.h>

#include "ml/bcc/bcc_server.hpp"
#include "ml/yolov3/yolov3_server.hpp"

template<typename T>
inf_server_config load_config(const T& root, const std::string& name)
{
    inf_server_config conf;
    auto obj = toml::find(root, name);
    conf.name = name;
    conf.xmodels = toml::find<std::vector<std::string>>(obj, "xmodels");
    int port = toml::find<int>(obj, "port");
    conf.address = "tcp://*:" + std::to_string(port);
    conf.max_batch_latency = toml::find<int>(obj, "max_batch_latency");
    conf.max_batch_size = toml::find<int>(obj, "max_batch_size");
    conf.num_workers = toml::find<int>(obj, "num_workers");
    return conf;
}

int main (int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(config, "config.toml", "Config file in TOML format");
    arg_end;

    const auto config_obj = toml::find(toml::parse(config), "ml");

    yolov3_server server0 { load_config(config_obj, "yolov3") };
    server0.start(true);

    bcc_server server1 { load_config(config_obj, "bcc") };
    server1.start();
}

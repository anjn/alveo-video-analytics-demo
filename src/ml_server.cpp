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
#include "ml/carclassification/carclassification_server.hpp"
#include "ml/yolov3/yolov3_server.hpp"
//#include "ml/yolov6/yolov6_server.hpp"

template<typename T>
inf_server_config load_config(const T& root, const std::string& name, const int default_max_batch_size)
{
    inf_server_config conf;

    if (root.contains(name))
    {
        auto obj = toml::find(root, name);
        conf.name = name;
        conf.xmodels = toml::find<std::vector<std::string>>(obj, "xmodels");
        int port = toml::find<int>(obj, "port");
        conf.address = "tcp://*:" + std::to_string(port);
        conf.max_batch_latency = toml::find<int>(obj, "max_batch_latency");
        conf.max_batch_size = toml::find<int>(obj, "max_batch_size");
        conf.num_workers = toml::find<int>(obj, "num_workers");

        if (default_max_batch_size > 0)
            conf.max_batch_size = default_max_batch_size;

        conf.enable = true;
    }
    else
    {
        conf.enable = false;
    }

    return conf;
}

int main (int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(config, "config.toml", "Config file in TOML format");
    arg_i(max_batch_size, -1, "This overwrites the setting in the config file");
    arg_end;

    const auto config_obj = toml::find(toml::parse(config), "ml");

    std::vector<inf_server_config> confs;

    yolov3_server server0 { load_config(config_obj, "yolov3", max_batch_size) };
    server0.start(true);

//    yolov6_server server1 { load_config(config_obj, "yolov6", max_batch_size) };
//    server1.start(true);

    carclassification_server server2 { load_config(config_obj, "carclassification", max_batch_size) };
    server2.start(true);

    bcc_server server3 { load_config(config_obj, "bcc", max_batch_size) };
    server3.start();
}

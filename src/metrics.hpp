#pragma once
#include <influxdb.hpp>

struct metrics_client_influx_db
{
    std::string id;

    influxdb_cpp::server_info server_info;

    metrics_client_influx_db(
        std::string id_,
        influxdb_cpp::server_info server_info_
    ) :
        id(id_),
        server_info(server_info_)
    {}

    void push(int count)
    {
        influxdb_cpp::builder()
            .meas("count")
            .tag("camera", id)
            .field("value", count)
            .post_http(server_info);
    }

    std::string get_id() const
    {
        return id;
    }
};

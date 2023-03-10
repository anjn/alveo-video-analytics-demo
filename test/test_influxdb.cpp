#include <arg/arg.h>
#include <influxdb.hpp>

int main(int argc, char** argv)
{
    arg_begin("", 0, 0);
    arg_s(ip, "127.0.0.1", "InfluxDB server ip");
    arg_i(port, 8086, "InfluxDB server port");
    arg_end;

    influxdb_cpp::server_info si(ip, port, "org", "token", "bucket");

    influxdb_cpp::builder()
        .meas("count")
        .tag("camera", "00")
        .field("value", 99)
        .post_http(si);

    // Bulk
    influxdb_cpp::builder()
        .meas("count")
        .tag("camera", "01")
        .field("value", 11)
        .meas("count")
        .tag("camera", "02")
        .field("value", 22)
        .post_http(si);
}

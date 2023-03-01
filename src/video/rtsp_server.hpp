#pragma once

extern "C" {
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/rtsp-server/rtsp-media.h>
}

struct rtsp_server
{
    std::string channel;
    const int port;
    const int device_index;
    const bool enable_vvas;
    int bitrate = 16000;

    GstRTSPServer *server;
    GstRTSPMediaFactory *factory;

    rtsp_server(
        const std::string& channel_ = "rtspsink",
        int port_ = 8554,
        int device_index_ = 0,
        bool enable_vvas_ = true
    ) :
        channel(channel_),
        port(port_),
        device_index(device_index_),
        enable_vvas(enable_vvas_)
    {}

    void start()
    {
        static int dev_idx = 1;

        if (device_index >= 0) dev_idx = device_index % 2;
        else dev_idx = (dev_idx + 1) % 2;

        g_print("rtsp_server: dev-idx = %d\n", dev_idx);

        const std::string pipeline_str =
            " ("
            " intervideosrc name=src"
            //" ! videorate"
            " ! video/x-raw, framerate=15/1"
            " ! queue"
            " ! vvas_xvcuenc dev-idx=" + std::to_string(dev_idx) + " target-bitrate=" + std::to_string(bitrate) + " max-bitrate=" + std::to_string(bitrate) + " b-frames=0 control-rate=3 scaling-list=0 rc-mode=false"
            " ! queue"
            " ! h264parse"
            " ! rtph264pay name=pay0 pt=96 config-interval=-1"
            " )"
            ;

        server = gst_rtsp_server_new();
        auto port_str = std::to_string(port);
        g_object_set(server, "service", port_str.c_str(), nullptr);
        auto mounts = gst_rtsp_server_get_mount_points(server);

        factory = gst_rtsp_media_factory_new();
        gst_rtsp_media_factory_set_launch(factory, pipeline_str.c_str());
        gst_rtsp_media_factory_set_shared(factory, true);

//        auto pool = gst_rtsp_address_pool_new();
//        gst_rtsp_address_pool_add_range (pool, "192.168.1.0", "192.168.1.255", 5000, 5010, 16);
//        gst_rtsp_media_factory_set_address_pool(factory, pool);
//        gst_rtsp_media_factory_set_protocols(factory, GST_RTSP_LOWER_TRANS_UDP_MCAST);
//        g_object_unref(pool);

        g_signal_connect(factory, "media-configure", (GCallback) rtsp_server::media_configure, this);
        gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
        g_object_unref(mounts);

        auto ret = gst_rtsp_server_attach(server, nullptr);
        assert(ret);
    }

    static void media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, rtsp_server* server)
    {
        g_print("rtsp_server::media_configure\n");

        //g_object_set_data_full(G_OBJECT(media), "rtsp-extra-data", server, (GDestroyNotify) rtsp_server::free);

        GstElement* element = gst_rtsp_media_get_element(media);
        assert(element);

        GstElement* src = gst_bin_get_by_name(GST_BIN(element), "src");
        assert(src);
        g_object_set(G_OBJECT(src), "channel", server->channel.c_str(), nullptr);
        g_object_set(G_OBJECT(src), "timeout", GST_SECOND * 10, nullptr);
        gst_object_unref(src);

        gst_object_unref(element);
    }

    static void free(rtsp_server* server)
    {
        g_print("rtsp_server::free\n");
    }
};

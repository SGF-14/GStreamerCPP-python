#ifndef GST_RTSP_STREAMER_H
#define GST_RTSP_STREAMER_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

class GstRtspStreamer {
public:
    GstRtspStreamer();
    ~GstRtspStreamer();

    bool initialize(const std::string& rtsp_url, bool low_latency = true);
    void start();
    void stop();
    bool is_running() const { return running.load(); }
    
    void set_frame_callback(std::function<void(const uint8_t*, size_t, int, int)> callback);
    
    void set_low_latency_mode(bool enable);
    void set_target_bitrate(int bitrate_kbps);
    void set_resolution(int width, int height);

private:
    GstElement* pipeline;
    GstElement* source;
    GstElement* depay;
    GstElement* parser;
    GstElement* decoder;
    GstElement* converter;
    GstElement* videoscale;
    GstElement* capsfilter;
    GstElement* encoder;
    GstElement* appsink;
    
    std::string rtsp_url;
    std::atomic<bool> running;
    std::atomic<bool> low_latency;
    std::thread message_thread;
    std::mutex callback_mutex;
    
    int target_width;
    int target_height;
    int target_bitrate;
    
    std::function<void(const uint8_t*, size_t, int, int)> frame_callback;
    
    void create_pipeline();
    void cleanup_pipeline();
    void handle_messages();
    static GstFlowReturn on_new_sample(GstAppSink* sink, gpointer user_data);
    static void on_pad_added(GstElement* element, GstPad* pad, gpointer data);
};

#endif
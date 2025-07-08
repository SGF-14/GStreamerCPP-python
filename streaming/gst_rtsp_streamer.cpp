#include "gst_rtsp_streamer.h"
#include <iostream>
#include <gst/video/video.h>

GstRtspStreamer::GstRtspStreamer() 
    : pipeline(nullptr), running(false), low_latency(true),
      target_width(1920), target_height(1080), target_bitrate(4000) {
    
    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }
}

GstRtspStreamer::~GstRtspStreamer() {
    stop();
    cleanup_pipeline();
}

bool GstRtspStreamer::initialize(const std::string& url, bool low_latency_mode) {
    rtsp_url = url;
    low_latency = low_latency_mode;
    
    create_pipeline();
    return pipeline != nullptr;
}

void GstRtspStreamer::create_pipeline() {
    pipeline = gst_pipeline_new("rtsp-pipeline");
    
    source = gst_element_factory_make("rtspsrc", "source");
    depay = gst_element_factory_make("rtph264depay", "depay");
    parser = gst_element_factory_make("h264parse", "parser");
    decoder = gst_element_factory_make("avdec_h264", "decoder");
    converter = gst_element_factory_make("videoconvert", "converter");
    videoscale = gst_element_factory_make("videoscale", "scale");
    capsfilter = gst_element_factory_make("capsfilter", "filter");
    encoder = gst_element_factory_make("x264enc", "encoder");
    appsink = gst_element_factory_make("appsink", "sink");
    
    if (!pipeline || !source || !depay || !parser || !decoder || 
        !converter || !videoscale || !capsfilter || !encoder || !appsink) {
        cleanup_pipeline();
        return;
    }
    
    g_object_set(source, "location", rtsp_url.c_str(), nullptr);
    g_object_set(source, "latency", low_latency ? 0 : 200, nullptr);
    g_object_set(source, "buffer-mode", low_latency ? 0 : 1, nullptr);
    g_object_set(source, "protocols", 0x00000004, nullptr);
    g_object_set(source, "tcp-timeout", 5000000, nullptr);
    
    if (low_latency) {
        g_object_set(source, "do-rtsp-keep-alive", TRUE, nullptr);
        g_object_set(decoder, "max-threads", 4, nullptr);
    }
    
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, target_width,
        "height", G_TYPE_INT, target_height,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        nullptr);
    g_object_set(capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);
    
    g_object_set(encoder, "tune", 0x00000004, nullptr);
    g_object_set(encoder, "speed-preset", 1, nullptr);
    g_object_set(encoder, "bitrate", target_bitrate, nullptr);
    g_object_set(encoder, "key-int-max", 30, nullptr);
    
    if (low_latency) {
        g_object_set(encoder, "threads", 4, nullptr);
        g_object_set(encoder, "sliced-threads", TRUE, nullptr);
        g_object_set(encoder, "sync-lookahead", 0, nullptr);
        g_object_set(encoder, "rc-lookahead", 0, nullptr);
        g_object_set(encoder, "bframes", 0, nullptr);
    }
    
    g_object_set(appsink, "emit-signals", TRUE, nullptr);
    g_object_set(appsink, "sync", FALSE, nullptr);
    g_object_set(appsink, "max-buffers", low_latency ? 1 : 3, nullptr);
    g_object_set(appsink, "drop", TRUE, nullptr);
    
    GstCaps* sink_caps = gst_caps_new_simple("video/x-h264",
        "stream-format", G_TYPE_STRING, "byte-stream",
        "alignment", G_TYPE_STRING, "au",
        nullptr);
    g_object_set(appsink, "caps", sink_caps, nullptr);
    gst_caps_unref(sink_caps);
    
    gst_bin_add_many(GST_BIN(pipeline), source, depay, parser, decoder,
                     converter, videoscale, capsfilter, encoder, appsink, nullptr);
    
    g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), depay);
    
    if (!gst_element_link_many(depay, parser, decoder, converter, 
                               videoscale, capsfilter, encoder, appsink, nullptr)) {
        cleanup_pipeline();
        return;
    }
    
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), this);
}

void GstRtspStreamer::cleanup_pipeline() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

void GstRtspStreamer::start() {
    if (!pipeline || running) return;
    
    running = true;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    message_thread = std::thread(&GstRtspStreamer::handle_messages, this);
}

void GstRtspStreamer::stop() {
    if (!running) return;
    
    running = false;
    
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
    
    if (message_thread.joinable()) {
        message_thread.join();
    }
}

void GstRtspStreamer::handle_messages() {
    GstBus* bus = gst_element_get_bus(pipeline);
    
    while (running) {
        GstMessage* msg = gst_bus_timed_pop(bus, GST_SECOND);
        if (!msg) continue;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError* err;
                gchar* debug;
                gst_message_parse_error(msg, &err, &debug);
                std::cerr << "Error: " << err->message << std::endl;
                g_error_free(err);
                g_free(debug);
                running = false;
                break;
            }
            case GST_MESSAGE_EOS:
                running = false;
                break;
            default:
                break;
        }
        
        gst_message_unref(msg);
    }
    
    gst_object_unref(bus);
}

void GstRtspStreamer::set_frame_callback(std::function<void(const uint8_t*, size_t, int, int)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    frame_callback = callback;
}

void GstRtspStreamer::set_low_latency_mode(bool enable) {
    low_latency = enable;
}

void GstRtspStreamer::set_target_bitrate(int bitrate_kbps) {
    target_bitrate = bitrate_kbps;
}

void GstRtspStreamer::set_resolution(int width, int height) {
    target_width = width;
    target_height = height;
}

GstFlowReturn GstRtspStreamer::on_new_sample(GstAppSink* sink, gpointer user_data) {
    GstRtspStreamer* streamer = static_cast<GstRtspStreamer*>(user_data);
    
    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample) return GST_FLOW_OK;
    
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        std::lock_guard<std::mutex> lock(streamer->callback_mutex);
        if (streamer->frame_callback) {
            GstCaps* caps = gst_sample_get_caps(sample);
            GstStructure* structure = gst_caps_get_structure(caps, 0);
            int width, height;
            gst_structure_get_int(structure, "width", &width);
            gst_structure_get_int(structure, "height", &height);
            
            streamer->frame_callback(map.data, map.size, width, height);
        }
        gst_buffer_unmap(buffer, &map);
    }
    
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void GstRtspStreamer::on_pad_added(GstElement* element, GstPad* pad, gpointer data) {
    GstElement* depay = GST_ELEMENT(data);
    GstPad* sink_pad = gst_element_get_static_pad(depay, "sink");
    
    if (gst_pad_is_linked(sink_pad)) {
        gst_object_unref(sink_pad);
        return;
    }
    
    gst_pad_link(pad, sink_pad);
    gst_object_unref(sink_pad);
}
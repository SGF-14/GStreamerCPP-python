#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "gst_rtsp_streamer.h"

namespace py = pybind11;

class PyGstRtspStreamer {
private:
    GstRtspStreamer streamer;
    py::function py_callback;
    
public:
    PyGstRtspStreamer() {}
    
    bool initialize(const std::string& rtsp_url, bool low_latency = true) {
        return streamer.initialize(rtsp_url, low_latency);
    }
    
    void start() {
        streamer.start();
    }
    
    void stop() {
        streamer.stop();
    }
    
    bool is_running() const {
        return streamer.is_running();
    }
    
    void set_frame_callback(py::function callback) {
        py_callback = callback;
        
        streamer.set_frame_callback([this](const uint8_t* data, size_t size, int width, int height) {
            if (py_callback) {
                py::gil_scoped_acquire acquire;
                
                py::array_t<uint8_t> frame({size}, {sizeof(uint8_t)}, data);
                
                try {
                    py_callback(frame, width, height);
                } catch (py::error_already_set& e) {
                    std::cerr << "Python callback error: " << e.what() << std::endl;
                }
            }
        });
    }
    
    void set_low_latency_mode(bool enable) {
        streamer.set_low_latency_mode(enable);
    }
    
    void set_target_bitrate(int bitrate_kbps) {
        streamer.set_target_bitrate(bitrate_kbps);
    }
    
    void set_resolution(int width, int height) {
        streamer.set_resolution(width, height);
    }
};

PYBIND11_MODULE(gst_streamer, m) {
    m.doc() = "GStreamer RTSP low-latency streaming module";
    
    py::class_<PyGstRtspStreamer>(m, "RtspStreamer")
        .def(py::init<>())
        .def("initialize", &PyGstRtspStreamer::initialize,
             py::arg("rtsp_url"),
             py::arg("low_latency") = true,
             "Initialize the RTSP streamer with given URL")
        .def("start", &PyGstRtspStreamer::start, "Start streaming")
        .def("stop", &PyGstRtspStreamer::stop, "Stop streaming")
        .def("is_running", &PyGstRtspStreamer::is_running, "Check if streamer is running")
        .def("set_frame_callback", &PyGstRtspStreamer::set_frame_callback,
             "Set callback function for receiving frames")
        .def("set_low_latency_mode", &PyGstRtspStreamer::set_low_latency_mode,
             "Enable/disable low latency mode")
        .def("set_target_bitrate", &PyGstRtspStreamer::set_target_bitrate,
             "Set target bitrate in kbps")
        .def("set_resolution", &PyGstRtspStreamer::set_resolution,
             "Set target resolution");
}
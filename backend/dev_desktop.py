import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '../streaming/build'))

import asyncio
import json
import logging
from aiohttp import web
from aiohttp_cors import setup, ResourceOptions
import gst_streamer
from video_bridge import VideoStreamHandler, WebRTCServer

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class VideoStreamingServer:
    def __init__(self):
        self.app = web.Application()
        self.video_handler = VideoStreamHandler()
        self.webrtc_server = WebRTCServer(self.video_handler)
        self.streamer = gst_streamer.RtspStreamer()
        self.current_rtsp_url = None
        
        self.setup_routes()
        self.setup_cors()
        
    def setup_routes(self):
        self.app.router.add_post('/api/start_stream', self.start_stream)
        self.app.router.add_post('/api/stop_stream', self.stop_stream)
        self.app.router.add_get('/api/stream_status', self.get_stream_status)
        self.app.router.add_post('/api/webrtc/offer', self.create_webrtc_offer)
        self.app.router.add_post('/api/webrtc/answer', self.set_webrtc_answer)
        self.app.router.add_post('/api/set_config', self.set_stream_config)
        
    def setup_cors(self):
        cors = setup(self.app, defaults={
            "*": ResourceOptions(
                allow_credentials=True,
                expose_headers="*",
                allow_headers="*",
                allow_methods="*"
            )
        })
        
        for route in list(self.app.router.routes()):
            cors.add(route)
    
    async def start_stream(self, request):
        try:
            data = await request.json()
            rtsp_url = data.get('rtsp_url')
            low_latency = data.get('low_latency', True)
            
            if not rtsp_url:
                return web.json_response({
                    'success': False,
                    'error': 'RTSP URL is required'
                }, status=400)
            
            if self.streamer.is_running():
                self.streamer.stop()
            
            self.streamer.set_frame_callback(self.video_handler.on_frame)
            
            if not self.streamer.initialize(rtsp_url, low_latency):
                return web.json_response({
                    'success': False,
                    'error': 'Failed to initialize streamer'
                }, status=500)
            
            self.streamer.start()
            self.current_rtsp_url = rtsp_url
            
            await asyncio.sleep(0.5)
            
            return web.json_response({
                'success': True,
                'message': 'Stream started successfully'
            })
            
        except Exception as e:
            logger.error(f"Error starting stream: {e}")
            return web.json_response({
                'success': False,
                'error': str(e)
            }, status=500)
    
    async def stop_stream(self, request):
        try:
            if self.streamer.is_running():
                self.streamer.stop()
                self.current_rtsp_url = None
            
            return web.json_response({
                'success': True,
                'message': 'Stream stopped successfully'
            })
            
        except Exception as e:
            logger.error(f"Error stopping stream: {e}")
            return web.json_response({
                'success': False,
                'error': str(e)
            }, status=500)
    
    async def get_stream_status(self, request):
        return web.json_response({
            'is_running': self.streamer.is_running(),
            'current_url': self.current_rtsp_url,
            'has_frame': self.video_handler.get_latest_frame() is not None
        })
    
    async def create_webrtc_offer(self, request):
        try:
            offer = await self.webrtc_server.create_offer()
            return web.json_response({
                'success': True,
                'offer': offer
            })
        except Exception as e:
            logger.error(f"Error creating WebRTC offer: {e}")
            return web.json_response({
                'success': False,
                'error': str(e)
            }, status=500)
    
    async def set_webrtc_answer(self, request):
        try:
            data = await request.json()
            pc_id = data.get('pc_id')
            answer = data.get('answer')
            
            if not pc_id or not answer:
                return web.json_response({
                    'success': False,
                    'error': 'pc_id and answer are required'
                }, status=400)
            
            await self.webrtc_server.set_answer(pc_id, answer)
            
            return web.json_response({
                'success': True,
                'message': 'Answer set successfully'
            })
        except Exception as e:
            logger.error(f"Error setting WebRTC answer: {e}")
            return web.json_response({
                'success': False,
                'error': str(e)
            }, status=500)
    
    async def set_stream_config(self, request):
        try:
            data = await request.json()
            
            if 'low_latency' in data:
                self.streamer.set_low_latency_mode(data['low_latency'])
            
            if 'bitrate' in data:
                self.streamer.set_target_bitrate(data['bitrate'])
            
            if 'width' in data and 'height' in data:
                self.streamer.set_resolution(data['width'], data['height'])
            
            return web.json_response({
                'success': True,
                'message': 'Configuration updated'
            })
        except Exception as e:
            logger.error(f"Error setting configuration: {e}")
            return web.json_response({
                'success': False,
                'error': str(e)
            }, status=500)
    
    async def cleanup(self, app):
        if self.streamer.is_running():
            self.streamer.stop()
        await self.webrtc_server.close_all()
    
    def run(self, host='0.0.0.0', port=8000):
        self.app.on_cleanup.append(self.cleanup)
        logger.info(f"Starting server on {host}:{port}")
        web.run_app(self.app, host=host, port=port)

if __name__ == '__main__':
    server = VideoStreamingServer()
    server.run()
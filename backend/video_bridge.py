import asyncio
import base64
import json
import numpy as np
from typing import Optional, Callable
import cv2
from aiortc import VideoStreamTrack, RTCPeerConnection, RTCSessionDescription
from av import VideoFrame
import fractions
import time
from queue import Queue, Empty
import threading

class VideoStreamHandler:
    def __init__(self):
        self.latest_frame: Optional[np.ndarray] = None
        self.frame_width = 0
        self.frame_height = 0
        self.frame_lock = threading.Lock()
        self.frame_event = threading.Event()
        
    def on_frame(self, frame_data: np.ndarray, width: int, height: int):
        h264_data = frame_data.tobytes()
        
        decoded_frame = self._decode_h264_frame(h264_data, width, height)
        if decoded_frame is not None:
            with self.frame_lock:
                self.latest_frame = decoded_frame
                self.frame_width = width
                self.frame_height = height
                self.frame_event.set()
    
    def _decode_h264_frame(self, h264_data: bytes, width: int, height: int) -> Optional[np.ndarray]:
        try:
            nalu_type = h264_data[4] & 0x1f
            
            if nalu_type == 7:
                return None
            
            packet = np.frombuffer(h264_data, dtype=np.uint8)
            return cv2.imdecode(packet, cv2.IMREAD_COLOR)
        except Exception as e:
            return None
    
    def get_latest_frame(self) -> Optional[tuple]:
        with self.frame_lock:
            if self.latest_frame is not None:
                return self.latest_frame.copy(), self.frame_width, self.frame_height
        return None

class WebRTCVideoTrack(VideoStreamTrack):
    def __init__(self, video_handler: VideoStreamHandler):
        super().__init__()
        self.video_handler = video_handler
        self.frame_count = 0
        
    async def recv(self):
        pts, time_base = await self.next_timestamp()
        
        frame_data = self.video_handler.get_latest_frame()
        if frame_data is None:
            frame = np.zeros((480, 640, 3), dtype=np.uint8)
            frame[:] = (64, 64, 64)
        else:
            frame, _, _ = frame_data
            
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        
        av_frame = VideoFrame.from_ndarray(frame, format="rgb24")
        av_frame.pts = pts
        av_frame.time_base = time_base
        
        self.frame_count += 1
        return av_frame

class WebRTCServer:
    def __init__(self, video_handler: VideoStreamHandler):
        self.video_handler = video_handler
        self.pcs = set()
        
    async def create_offer(self):
        pc = RTCPeerConnection()
        self.pcs.add(pc)
        
        @pc.on("connectionstatechange")
        async def on_connectionstatechange():
            if pc.connectionState == "failed" or pc.connectionState == "closed":
                await pc.close()
                self.pcs.discard(pc)
        
        video_track = WebRTCVideoTrack(self.video_handler)
        pc.addTrack(video_track)
        
        offer = await pc.createOffer()
        await pc.setLocalDescription(offer)
        
        return {
            "sdp": pc.localDescription.sdp,
            "type": pc.localDescription.type,
            "pc_id": id(pc)
        }
    
    async def set_answer(self, pc_id: int, answer: dict):
        pc = next((p for p in self.pcs if id(p) == pc_id), None)
        if pc:
            await pc.setRemoteDescription(
                RTCSessionDescription(sdp=answer["sdp"], type=answer["type"])
            )
    
    async def close_all(self):
        for pc in self.pcs:
            await pc.close()
        self.pcs.clear()
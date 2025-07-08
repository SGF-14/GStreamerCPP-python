<script>
    import { onMount, onDestroy } from 'svelte';
    
    export let rtspUrl = '';
    
    let videoElement;
    let pc;
    let pcId;
    let isConnecting = false;
    let isConnected = false;
    let stats = {
      fps: 0,
      bitrate: 0,
      latency: 0,
      packetsLost: 0
    };
    let statsInterval;
    
    async function startStream() {
      if (!rtspUrl) return;
      
      isConnecting = true;
      
      try {
        const response = await fetch('/api/start_stream', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ 
            rtsp_url: rtspUrl,
            low_latency: true 
          })
        });
        
        const result = await response.json();
        
        if (!result.success) {
          throw new Error(result.error);
        }
        
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        await setupWebRTC();
        
      } catch (error) {
        console.error('Failed to start stream:', error);
        alert('Failed to start stream: ' + error.message);
        isConnecting = false;
      }
    }
    
    async function setupWebRTC() {
      try {
        const offerResponse = await fetch('/api/webrtc/offer', {
          method: 'POST'
        });
        
        const offerResult = await offerResponse.json();
        
        if (!offerResult.success) {
          throw new Error(offerResult.error);
        }
        
        const { offer, pc_id } = offerResult.offer;
        pcId = pc_id;
        
        pc = new RTCPeerConnection({
          iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
        });
        
        pc.ontrack = (event) => {
          if (videoElement && event.streams[0]) {
            videoElement.srcObject = event.streams[0];
          }
        };
        
        pc.onconnectionstatechange = () => {
          console.log('Connection state:', pc.connectionState);
          if (pc.connectionState === 'connected') {
            isConnected = true;
            isConnecting = false;
            startStatsCollection();
          } else if (pc.connectionState === 'failed' || pc.connectionState === 'disconnected') {
            isConnected = false;
            stopStatsCollection();
          }
        };
        
        await pc.setRemoteDescription({ type: 'offer', sdp: offer });
        
        const answer = await pc.createAnswer();
        await pc.setLocalDescription(answer);
        
        const answerResponse = await fetch('/api/webrtc/answer', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            pc_id: pcId,
            answer: {
              type: answer.type,
              sdp: answer.sdp
            }
          })
        });
        
        const answerResult = await answerResponse.json();
        
        if (!answerResult.success) {
          throw new Error(answerResult.error);
        }
        
      } catch (error) {
        console.error('WebRTC setup failed:', error);
        isConnecting = false;
        throw error;
      }
    }
    
    async function stopStream() {
      if (pc) {
        pc.close();
        pc = null;
      }
      
      stopStatsCollection();
      
      try {
        await fetch('/api/stop_stream', { method: 'POST' });
      } catch (error) {
        console.error('Failed to stop stream:', error);
      }
      
      isConnected = false;
      isConnecting = false;
    }
    
    function startStatsCollection() {
      statsInterval = setInterval(async () => {
        if (!pc) return;
        
        const stats = await pc.getStats();
        let videoStats = null;
        
        stats.forEach(report => {
          if (report.type === 'inbound-rtp' && report.kind === 'video') {
            videoStats = report;
          }
        });
        
        if (videoStats) {
          const now = Date.now();
          
          if (window.lastStats) {
            const timeDiff = (now - window.lastStatsTime) / 1000;
            const bytesDiff = videoStats.bytesReceived - window.lastStats.bytesReceived;
            const framesDiff = videoStats.framesDecoded - window.lastStats.framesDecoded;
            
            stats = {
              fps: Math.round(framesDiff / timeDiff),
              bitrate: Math.round((bytesDiff * 8) / timeDiff / 1000),
              latency: videoStats.jitterBufferDelay 
                ? Math.round(videoStats.jitterBufferDelay * 1000) 
                : 0,
              packetsLost: videoStats.packetsLost || 0
            };
          }
          
          window.lastStats = videoStats;
          window.lastStatsTime = now;
        }
      }, 1000);
    }
    
    function stopStatsCollection() {
      if (statsInterval) {
        clearInterval(statsInterval);
        statsInterval = null;
      }
      window.lastStats = null;
      window.lastStatsTime = null;
    }
    
    onDestroy(() => {
      stopStream();
    });
  </script>
  
  <div class="video-player">
    <div class="controls">
      <input 
        type="text" 
        bind:value={rtspUrl}
        placeholder="rtsp://192.168.1.100:554/stream"
        disabled={isConnecting || isConnected}
      />
      
      {#if !isConnected && !isConnecting}
        <button on:click={startStream} disabled={!rtspUrl}>
          Start Stream
        </button>
      {:else if isConnecting}
        <button disabled>
          Connecting...
        </button>
      {:else}
        <button on:click={stopStream} class="stop">
          Stop Stream
        </button>
      {/if}
    </div>
    
    <div class="video-container">
      <video 
        bind:this={videoElement}
        autoplay
        playsinline
        muted
      ></video>
      
      {#if isConnected}
        <div class="stats">
          <div>FPS: {stats.fps}</div>
          <div>Bitrate: {stats.bitrate} kbps</div>
          <div>Latency: {stats.latency} ms</div>
          <div>Packets Lost: {stats.packetsLost}</div>
        </div>
      {/if}
    </div>
  </div>
  
  <style>
    .video-player {
      width: 100%;
      max-width: 1200px;
      margin: 0 auto;
    }
    
    .controls {
      display: flex;
      gap: 10px;
      margin-bottom: 20px;
    }
    
    input {
      flex: 1;
      padding: 10px;
      font-size: 16px;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    
    button {
      padding: 10px 20px;
      font-size: 16px;
      background: #007bff;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      transition: background 0.3s;
    }
    
    button:hover:not(:disabled) {
      background: #0056b3;
    }
    
    button:disabled {
      background: #ccc;
      cursor: not-allowed;
    }
    
    button.stop {
      background: #dc3545;
    }
    
    button.stop:hover {
      background: #c82333;
    }
    
    .video-container {
      position: relative;
      width: 100%;
      background: #000;
      border-radius: 8px;
      overflow: hidden;
    }
    
    video {
      width: 100%;
      height: auto;
      display: block;
    }
    
    .stats {
      position: absolute;
      top: 10px;
      right: 10px;
      background: rgba(0, 0, 0, 0.7);
      color: white;
      padding: 10px;
      border-radius: 4px;
      font-family: monospace;
      font-size: 14px;
    }
    
    .stats div {
      margin: 2px 0;
    }
  </style>
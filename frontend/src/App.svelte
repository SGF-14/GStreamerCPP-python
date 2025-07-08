<script>
    import VideoPlayer from './lib/VideoPlayer.svelte';
    
    let showAdvanced = false;
    let config = {
      lowLatency: true,
      bitrate: 4000,
      width: 1920,
      height: 1080
    };
    
    async function updateConfig() {
      try {
        const response = await fetch('/api/set_config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            low_latency: config.lowLatency,
            bitrate: config.bitrate,
            width: config.width,
            height: config.height
          })
        });
        
        const result = await response.json();
        
        if (result.success) {
          alert('Configuration updated successfully');
        } else {
          alert('Failed to update configuration: ' + result.error);
        }
      } catch (error) {
        console.error('Failed to update config:', error);
        alert('Failed to update configuration');
      }
    }
  </script>
  
  <main>
    <h1>Low Latency RTSP Video Streamer</h1>
    
    <div class="description">
      <p>GStreamer-powered video streaming with ultra-low latency, similar to QGroundControl</p>
    </div>
    
    <VideoPlayer />
    
    <div class="advanced-section">
      <button 
        class="toggle-advanced" 
        on:click={() => showAdvanced = !showAdvanced}
      >
        {showAdvanced ? 'Hide' : 'Show'} Advanced Settings
      </button>
      
      {#if showAdvanced}
        <div class="advanced-settings">
          <h3>Stream Configuration</h3>
          
          <div class="setting">
            <label>
              <input 
                type="checkbox" 
                bind:checked={config.lowLatency}
              />
              Low Latency Mode
            </label>
          </div>
          
          <div class="setting">
            <label>
              Bitrate (kbps):
              <input 
                type="number" 
                bind:value={config.bitrate}
                min="500"
                max="50000"
                step="500"
              />
            </label>
          </div>
          
          <div class="setting">
            <label>
              Resolution:
              <select 
                on:change={(e) => {
                  const [w, h] = e.target.value.split('x').map(Number);
                  config.width = w;
                  config.height = h;
                }}
              >
                <option value="1920x1080">1920x1080 (1080p)</option>
                <option value="1280x720">1280x720 (720p)</option>
                <option value="640x480">640x480 (480p)</option>
                <option value="3840x2160">3840x2160 (4K)</option>
              </select>
            </label>
          </div>
          
          <button on:click={updateConfig}>Apply Configuration</button>
        </div>
      {/if}
    </div>
  </main>
  
  <style>
    :global(body) {
      margin: 0;
      padding: 0;
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
      background: #f5f5f5;
    }
    
    main {
      padding: 20px;
      max-width: 1400px;
      margin: 0 auto;
    }
    
    h1 {
      text-align: center;
      color: #333;
      margin-bottom: 10px;
    }
    
    .description {
      text-align: center;
      color: #666;
      margin-bottom: 30px;
    }
    
    .advanced-section {
      margin-top: 40px;
      text-align: center;
    }
    
    .toggle-advanced {
      padding: 8px 16px;
      background: #6c757d;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
    }
    
    .toggle-advanced:hover {
      background: #5a6268;
    }
    
    .advanced-settings {
      margin-top: 20px;
      padding: 20px;
      background: white;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      text-align: left;
      max-width: 500px;
      margin-left: auto;
      margin-right: auto;
    }
    
    .advanced-settings h3 {
      margin-top: 0;
      color: #333;
    }
    
    .setting {
      margin: 15px 0;
    }
    
    .setting label {
      display: block;
      color: #555;
      font-size: 14px;
    }
    
    .setting input[type="number"],
    .setting select {
      margin-top: 5px;
      padding: 5px;
      width: 200px;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    
    .setting input[type="checkbox"] {
      margin-right: 8px;
    }
    
    .advanced-settings button {
      margin-top: 15px;
      padding: 8px 16px;
      background: #28a745;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }
    
    .advanced-settings button:hover {
      background: #218838;
    }
  </style>
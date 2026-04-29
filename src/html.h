#ifndef HTML_H
#define HTML_H

// PROGMEM strings for web interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
  <head>
    <title>Pool Monitor</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
      :root {
        --primary-start: #667eea;
        --primary-end: #764ba2;
        --accent-start: #f093fb;
        --accent-end: #f5576c;
        --primary-color: #667eea;
      }
      
      /* Theme: Ocean (default) */
      body.theme-ocean {
        --primary-start: #667eea;
        --primary-end: #764ba2;
        --accent-start: #f093fb;
        --accent-end: #f5576c;
        --primary-color: #667eea;
      }
      
      /* Theme: Forest */
      body.theme-forest {
        --primary-start: #11998e;
        --primary-end: #38ef7d;
        --accent-start: #56ab2f;
        --accent-end: #a8e063;
        --primary-color: #11998e;
      }
      
      /* Theme: Sand (Earth) */
      body.theme-sand {
        --primary-start: #c79a6c;
        --primary-end: #8b6f47;
        --accent-start: #d4a574;
        --accent-end: #b8956a;
        --primary-color: #c79a6c;
      }
      
      /* Theme: Night */
      body.theme-night {
        --primary-start: #2c3e50;
        --primary-end: #3498db;
        --accent-start: #8e44ad;
        --accent-end: #3498db;
        --primary-color: #3498db;
      }
      
      /* Theme: Terracotta (Earth) */
      body.theme-terracotta {
        --primary-start: #c16e4f;
        --primary-end: #8b5a3c;
        --accent-start: #d4845c;
        --accent-end: #a8694e;
        --primary-color: #c16e4f;
      }
      
      * { margin: 0; padding: 0; box-sizing: border-box; }
      body { 
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
        background: linear-gradient(135deg, var(--primary-start) 0%, var(--primary-end) 100%);
        min-height: 100vh;
        padding: 20px;
        color: #333;
        transition: background 0.5s ease;
        position: relative;
        overflow-x: hidden;
      }
      /* Animated pool water caustic light effect */
      body::before {
        content: '';
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        background: 
          radial-gradient(ellipse at 30% 40%, rgba(255,255,255,0.15) 0%, transparent 50%),
          radial-gradient(ellipse at 70% 60%, rgba(255,255,255,0.12) 0%, transparent 50%),
          radial-gradient(ellipse at 50% 80%, rgba(255,255,255,0.1) 0%, transparent 40%);
        pointer-events: none;
        z-index: 0;
        animation: waterCaustics 15s ease-in-out infinite;
      }
      /* Second water layer for depth */
      body::after {
        content: '';
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        background: 
          radial-gradient(circle at 20% 30%, rgba(255,255,255,0.08) 0%, transparent 60%),
          radial-gradient(circle at 80% 70%, rgba(255,255,255,0.06) 0%, transparent 60%);
        pointer-events: none;
        z-index: 0;
        animation: waterCaustics 12s ease-in-out infinite reverse;
      }
      @keyframes waterCaustics {
        0%, 100% { 
          transform: translate(0, 0) scale(1);
          opacity: 0.8;
        }
        25% { 
          transform: translate(-3%, 2%) scale(1.05);
          opacity: 1;
        }
        50% { 
          transform: translate(2%, -2%) scale(0.95);
          opacity: 0.7;
        }
        75% { 
          transform: translate(-2%, 3%) scale(1.02);
          opacity: 0.9;
        }
      }
      .container {
        max-width: 800px;
        margin: 0 auto;
        position: relative;
        z-index: 1;
      }
      .header {
        background: white;
        border-radius: 16px;
        padding: 24px;
        margin-bottom: 20px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.1);
      }
      .header h1 {
        font-size: 28px;
        color: var(--primary-color);
        margin-bottom: 12px;
        font-weight: 600;
      }
      .status-bar {
        display: flex;
        gap: 16px;
        flex-wrap: wrap;
        align-items: center;
        font-size: 14px;
        color: #666;
      }
      .status-item {
        display: flex;
        align-items: center;
        gap: 6px;
      }
      .status-dot {
        width: 12px;
        height: 12px;
        background-color: #f00;
        border-radius: 50%;
        animation: pulse 2s ease-in-out infinite;
      }
      @keyframes pulse {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.5; }
      }
      .temp-display {
        font-size: 18px;
        font-weight: 600;
        color: var(--primary-color);
      }
      .card {
        background: white;
        border-radius: 16px;
        padding: 24px;
        margin-bottom: 20px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.1);
        transition: transform 0.2s, box-shadow 0.2s;
      }
      .card:hover {
        transform: translateY(-2px);
        box-shadow: 0 12px 48px rgba(0,0,0,0.15);
      }
      .card h2 {
        font-size: 20px;
        margin-bottom: 16px;
        color: #333;
        font-weight: 600;
      }
      .pump-controls {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
        gap: 12px;
        margin-bottom: 16px;
      }
      .button {
        padding: 16px;
        font-size: 16px;
        font-weight: 500;
        text-align: center;
        color: #fff;
        background: linear-gradient(135deg, var(--primary-start) 0%, var(--primary-end) 100%);
        border: none;
        border-radius: 12px;
        cursor: pointer;
        transition: all 0.3s ease;
        box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: transparent;
      }  
      .button:hover {
        transform: translateY(-2px);
        box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);
      }
      .button:active {
        transform: translateY(0);
        box-shadow: 0 2px 10px rgba(102, 126, 234, 0.4);
      }
      .button.secondary {
        background: linear-gradient(135deg, var(--accent-start) 0%, var(--accent-end) 100%);
        box-shadow: 0 4px 15px rgba(245, 87, 108, 0.4);
      }
      .button.secondary:hover {
        box-shadow: 0 6px 20px rgba(245, 87, 108, 0.6);
      }
      .speed-display {
        text-align: center;
        padding: 16px;
        background: linear-gradient(135deg, var(--accent-start) 0%, var(--accent-end) 100%);
        border-radius: 12px;
        color: white;
        font-size: 18px;
        font-weight: 600;
        margin-bottom: 16px;
      }
      .status-badge {
        display: inline-block;
        padding: 8px 16px;
        border-radius: 20px;
        font-weight: 600;
        font-size: 16px;
      }
      .status-ok { background: #10b981; color: white; }
      .status-error { background: #ef4444; color: white; }
      .time-settings {
        display: grid;
        gap: 12px;
      }
      .time-row {
        display: grid;
        grid-template-columns: 1fr auto;
        gap: 12px;
        align-items: center;
        padding: 12px;
        background: #f9fafb;
        border-radius: 8px;
      }
      .time-row label {
        font-weight: 500;
        color: #666;
      }
      .time-row span {
        font-weight: 600;
        color: var(--primary-color);
        font-size: 18px;
      }
      .time-input-group {
        display: grid;
        gap: 12px;
        margin-top: 16px;
      }
      .input-wrapper {
        display: flex;
        flex-direction: column;
        gap: 6px;
      }
      .input-wrapper label {
        font-size: 14px;
        font-weight: 500;
        color: #666;
      }
      input[type="number"] {
        padding: 12px;
        border: 2px solid #e5e7eb;
        border-radius: 8px;
        font-size: 16px;
        transition: border-color 0.3s;
      }
      input[type="number"]:focus {
        outline: none;
        border-color: var(--primary-color);
      }
      .button-group {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
        gap: 12px;
        margin-top: 16px;
      }
      .theme-selector {
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
        margin-top: 12px;
      }
      .theme-button {
        width: 40px;
        height: 40px;
        border-radius: 50%;
        border: 3px solid white;
        cursor: pointer;
        transition: transform 0.2s, box-shadow 0.2s;
        box-shadow: 0 2px 8px rgba(0,0,0,0.2);
      }
      .theme-button:hover {
        transform: scale(1.1);
        box-shadow: 0 4px 12px rgba(0,0,0,0.3);
      }
      .theme-button.active {
        border-color: #333;
        box-shadow: 0 4px 16px rgba(0,0,0,0.4);
        transform: scale(1.15);
      }
      .theme-button.ocean {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      }
      .theme-button.forest {
        background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
      }
      .theme-button.sand {
        background: linear-gradient(135deg, #c79a6c 0%, #8b6f47 100%);
      }
      .theme-button.night {
        background: linear-gradient(135deg, #2c3e50 0%, #3498db 100%);
      }
      .theme-button.terracotta {
        background: linear-gradient(135deg, #c16e4f 0%, #8b5a3c 100%);
      }
      @media (max-width: 600px) {
        .pump-controls {
          grid-template-columns: 1fr 1fr;
        }
        .header h1 { font-size: 24px; }
        .status-bar { font-size: 12px; }
      }
     </style>
  </head>
  <body>
    <div class="container">
      <div class="header">
        <h1>🏊 Pool Monitor</h1>
        <div class="status-bar">
          <div class="status-item">
            <span class="status-dot" id="mydot"></span>
            <span>Connected</span>
          </div>
          <div class="status-item" id="rssi">📶 RSSI -65 | ⏰ 08:00:00 14.06.2022</div>
          <div class="status-item">
            <span class="temp-display" id="status_temp">🌡️ -999°C</span>
          </div>
        </div>
        <div class="theme-selector">
          <div class="theme-button ocean active" onclick="setTheme('ocean')" title="Ocean"></div>
          <div class="theme-button forest" onclick="setTheme('forest')" title="Forest"></div>
          <div class="theme-button sand" onclick="setTheme('sand')" title="Sand"></div>
          <div class="theme-button night" onclick="setTheme('night')" title="Night"></div>
          <div class="theme-button terracotta" onclick="setTheme('terracotta')" title="Terracotta"></div>
        </div>
      </div>
      <div class="card">
        <h2>⚙️ Pump Controls</h2>
        <div class="speed-display" id="poolPumpSpeed">Pump Speed LOW</div>
        <div class="pump-controls">
          <button class="button" onmousedown="toggleCheckbox('LowOn');" ontouchstart="toggleCheckbox('LowOn');" onmouseup="toggleCheckbox('LowOff');" ontouchend="toggleCheckbox('LowOff');">🐢 Low Speed</button>
          <button class="button" onmousedown="toggleCheckbox('MedOn');" ontouchstart="toggleCheckbox('MedOn');" onmouseup="toggleCheckbox('MedOff');" ontouchend="toggleCheckbox('MedOff');">🚶 Med Speed</button>
          <button class="button" onmousedown="toggleCheckbox('HighOn');" ontouchstart="toggleCheckbox('HighOn');" onmouseup="toggleCheckbox('HighOff');" ontouchend="toggleCheckbox('HighOff');">🚀 High Speed</button>
          <button class="button secondary" onmousedown="toggleCheckbox('StopOn');" ontouchstart="toggleCheckbox('StopOn');" onmouseup="toggleCheckbox('StopOff');" ontouchend="toggleCheckbox('StopOff');">⏹️ Stop</button>
        </div>
      </div>

      <div class="card">
        <h2>💧 Pool Status</h2>
        <div style="text-align: center;">
          <span class="status-badge status-ok" id="poolRelaxState">OK</span>
        </div>
      </div>

      <div class="card">
        <h2>⏰ Schedule Settings</h2>
        <div class="time-settings">
          <div class="time-row">
            <label>Pump Start Time</label>
            <span id="timeOn">09:00</span>
          </div>
          <div class="time-row">
            <label>Pump Stop Time</label>
            <span id="timeOff">18:00</span>
          </div>
        </div>
        
        <div class="time-input-group">
          <div class="input-wrapper">
            <label for="ontime">Set Start Hour (0-23)</label>
            <input type="number" id="ontime" min="0" max="23" placeholder="6" onchange="changeOnTime(value)">
          </div>
          <div class="input-wrapper">
            <label for="offtime">Set Stop Hour (0-23)</label>
            <input type="number" id="offtime" min="0" max="23" placeholder="18" onchange="changeOffTime(value)">
          </div>
        </div>
        
        <div class="button-group">
          <button class="button" onmouseup="setTime();" ontouchend="setTime();">💾 Save Schedule</button> 
          <button class="button secondary" onclick="logoutButton()">🚪 Logout</button>
        </div>
      </div>
    </div>

<script>
 // Theme management
 function setTheme(themeName) {
   // Remove all theme classes
   document.body.classList.remove('theme-ocean', 'theme-forest', 'theme-sand', 'theme-night', 'theme-terracotta');
   
   // Add new theme class
   document.body.classList.add('theme-' + themeName);
   
   // Update active state on buttons
   document.querySelectorAll('.theme-button').forEach(btn => {
     btn.classList.remove('active');
   });
   document.querySelector('.theme-button.' + themeName).classList.add('active');
   
   // Save theme preference in localStorage
   localStorage.setItem('poolMonitorTheme', themeName);
 }
 
 // Load saved theme on page load
 function loadSavedTheme() {
   const savedTheme = localStorage.getItem('poolMonitorTheme') || 'ocean';
   setTheme(savedTheme);
 }
 
 // Load theme when page loads
 loadSavedTheme();
 
 var server_running = true;
 var pumpSpeed;
 var RelaxStatus;
 var timeon = 6;  // Default values
 var timeoff = 18;
 var rssi;
 var hh;
 var mm;
 var ss;
 var dd;
 var md;
 var yy;
 var temperatur;
 
 function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
}
 
 function toggleCheckbox(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/update?button="+x, true);
     xhr.send();
     console.log('Button pressed '+x);

   
      
     if (x=="LowOff") 
    {
      document.getElementById("poolPumpSpeed").textContent = "Pump Speed LOW";
    }
    else if(x=="HighOff") 
    {
     document.getElementById("poolPumpSpeed").textContent = "Pump Speed HIGH";   
    }
     else if(x=="MedOff") 
    {
     document.getElementById("poolPumpSpeed").textContent = "Pump Speed Med";   
    }

      else if(x=="StopOff") 
    {
     document.getElementById("poolPumpSpeed").textContent = "Pump Stop";   
    }
     
   }

  function setTime(){
     var xhr = new XMLHttpRequest();
     const onTime = document.getElementById('ontime').value;
     const offTime = document.getElementById('offtime').value;
     
     // Use current values if fields are empty
     const finalOnTime = onTime !== '' ? onTime : timeon;
     const finalOffTime = offTime !== '' ? offTime : timeoff;
     
     console.log(finalOnTime);
     console.log(finalOffTime);
     
     if ((!isNaN(parseInt(finalOnTime))) && (!isNaN(parseInt(finalOffTime)))){
      
     xhr.open("GET", "/timeAdjust?onTime="+finalOnTime+"&offTime="+finalOffTime, true);
     xhr.send();
    
    }
    else
    console.log("Error");
   }
   
   function changeOnTime(x) {
   console.log(x);
   document.getElementById("timeOn").textContent = x;
   }
   
   function changeOffTime(x) {
   console.log(x);
   document.getElementById("timeOff").textContent = x;
   }
   
   
   function refreshState() {
  console.log('Running interval');

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
  
    if (this.readyState == 4 && this.status == 200) {
     const obj = JSON.parse(this.responseText);
     server_running=true;
     console.log(obj.poolRelaxStatus);
     console.log(obj.pumpSpeed);
     pumpSpeed = obj.pumpSpeed;
     RelaxStatus = obj.poolRelaxStatus;
     timeon=obj.onTime;
     timeoff=obj.offTime;
     rssi=obj.rssi;
     hh=obj.hh;
     mm=obj.mm;
     ss=obj.ss;
     dd=obj.dd;
     md=obj.md;
     yy=obj.yy;
     temperatur=obj.currentTemp;

if (RelaxStatus==1) 
    {
      document.getElementById("poolRelaxState").textContent = "OK";
      document.getElementById("poolRelaxState").className = "status-badge status-ok";
    }
    else if(RelaxStatus==0) 
    {
      document.getElementById("poolRelaxState").textContent = "ERROR";
      document.getElementById("poolRelaxState").className = "status-badge status-error";
    }

if (pumpSpeed==0) 
    {
      document.getElementById("poolPumpSpeed").textContent = "Pump Speed LOW";
    }
    else if(pumpSpeed==1) 
    {
     document.getElementById("poolPumpSpeed").textContent = "Pump Speed HIGH";   
    }
     else if(pumpSpeed==2) 
    {
       document.getElementById("poolPumpSpeed").textContent = "Pump Speed Not set"; 
    }
     else if(pumpSpeed==3) 
    {
       document.getElementById("poolPumpSpeed").textContent = "Pump Speed Med"; 
    }
     else if(pumpSpeed==4) 
    {
       document.getElementById("poolPumpSpeed").textContent = "Pump Speed Stop"; 
    }

    document.getElementById("timeOn").textContent = timeon + ":00";
    document.getElementById("timeOff").textContent = timeoff + ":00"; 
 
    document.getElementById("rssi").textContent = "📶 RSSI " + rssi + " | ⏰ " + hh + ":" + mm + ":" + ss + " " + dd + "." + md + "." + yy;
    document.getElementById("status_temp").textContent = "🌡️ " + temperatur + "°C";

  }
 };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}

// Fetch immediately on load (avoids 2 s flicker of placeholder values),
// then poll every 2 s.
refreshState();
setInterval(refreshState, 2000);

setInterval(function ( ) {
  if (server_running == true) {
    document.getElementById("mydot").style.backgroundColor = "green";
    server_running = false;
  } else {
    document.getElementById("mydot").style.backgroundColor = "red";
  }
}, 1000);

  </script>
  </body>
</html>
)rawliteral";


const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .card {
      background: white;
      border-radius: 16px;
      padding: 40px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.1);
      text-align: center;
      max-width: 400px;
    }
    h1 { color: #667eea; margin-bottom: 16px; font-size: 24px; }
    p { color: #666; line-height: 1.6; margin-bottom: 12px; }
    a { 
      color: #667eea; 
      text-decoration: none; 
      font-weight: 600;
      border-bottom: 2px solid #667eea;
    }
    a:hover { color: #764ba2; border-color: #764ba2; }
  </style>
</head>
<body>
  <div class="card">
    <h1>🚪 Logged Out</h1>
    <p>You have been successfully logged out.</p>
    <p><a href="/">← Return to homepage</a></p>
    <p style="margin-top: 20px; font-size: 14px;"><strong>Note:</strong> Close all browser tabs to complete the logout process.</p>
  </div>
</body>
</html>
)rawliteral";

#endif // HTML_H
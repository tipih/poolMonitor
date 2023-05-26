#include <Arduino.h>
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
  <head>
    <title>ESP Pump control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
      body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px;background-color: #383e42;}
      .button {
        padding: 10px 20px;
        width: 250px;
        margin: 5px;
        font-size: 24px;
        text-align: center;
        outline: none;
        color: #fff;
        background-color: #2f4468;
        border: none;
        border-radius: 5px;
        box-shadow: 0 6px #999;
        cursor: pointer;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0,0,0,0);
      }  
      .button:hover {background-color: #1f2e45}
      .button:active {
        background-color: #1f2e45;
        box-shadow: 0 4px #666;
        transform: translateY(2px);
      }
      
      .card {
  box-shadow: 0 4px 8px 0 rgba(0,0,0,0.9);
  transition: 0.3s;
  width: 90%;
  padding: 1px 1px 10px 10px;
  
  border-radius: 5px;
  margin: 0 auto;
 
}

.card:hover {
  box-shadow: 0 8px 16px 0 rgba(0,0,0,0.5);
}
  
  .dot {
    position: absolute;
    top: 5px;
    left: 5px;
  height: 10px;
  width: 10px;
  background-color: #f00;
  border-radius: 50%;
  display: inline-block;
}
  </style>
  </head>
  <body>
  
  <h1  style="color:#999999;"> "ESP Pump control </h1>
  <h5  style="color:#ffffff;" id="rssi">RSSI -65 Time 08:00:00 14.06.2022  </h5>
  <h5 class="dot" id="mydot"></h5>
  <h5  id="status_temp">-999 </h5>
    
    <button class="button" onmousedown="toggleCheckbox('LowOn');" ontouchstart="toggleCheckbox('LowOn');" onmouseup="toggleCheckbox('LowOff');" ontouchend="toggleCheckbox('LowOff');">Pump low speed</button>
    
    <button class="button" onmousedown="toggleCheckbox('MedOn');" ontouchstart="toggleCheckbox('MedOn');" onmouseup="toggleCheckbox('MedOff');" ontouchend="toggleCheckbox('MedOff');">Pump med speed</button>

    <button class="button" onmousedown="toggleCheckbox('HighOn');" ontouchstart="toggleCheckbox('HighOn');" onmouseup="toggleCheckbox('HighOff');" ontouchend="toggleCheckbox('HighOff');">Pump high speed</button>
  
    <button class="button" onmousedown="toggleCheckbox('StopOn');" ontouchstart="toggleCheckbox('StopOn');" onmouseup="toggleCheckbox('StopOff');" ontouchend="toggleCheckbox('StopOff');">Pump Stop</button>

  <h2 style="color:#999999;" id="poolPumpSpeed">Pump Speed LOW</h2>

<div class="card">
 
  <div class="container">
    <h4><b style="color:#999999;">Pool Relax status</b></h4> 
    <p  style="color:#dddddd;" id="poolRelaxState">OK</p>
  </div>
 </div>

  
<div class="card">  
   <div class="container">
  
  <h4><b style="color:#999999;">Time Settings</b></h4> 
    
  <div style="display: flex; justify-content: space-between;">
  <p style="flex-basis: 49.5%; background-color: papayawhip;">Pump Time On.</p>
  <p style="flex-basis: 49.5%; background-color: palegoldenrod;" id="timeOn">09</p>
</div>
   <div style="display: flex; justify-content: space-between;">
  <p style="flex-basis: 49.5%; background-color: papayawhip;">Pump Time Off.</p>
  <p style="flex-basis: 49.5%; background-color: palegoldenrod;" id="timeOff">18</p>
</div> 
    
 
    <h4 id="pumpontime">
    Pump on time
        <input type="number" id="ontime" name="tentacles"
       min="00" max="23" onchange="changeOnTime(value)">
    </h4>   
    <h4 id="pumpofftime">
    Pump off time
    <input type="number" id="offtime" name="tentacles"
       min="00" max="23" onchange="changeOffTime(value)">
       </h4>

    <button class="button" onmouseup="setTime();" ontouchend="toggleCheckbox();">SetTime</button> 
    <button class="button" onclick="logoutButton()">Logout</button> 
  </div>
 </div>

<script>
 var server_running = true;
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
      document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed LOW";
    }
    else if(x=="HighOff") 
    {
     document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed HIGH";   
    }
     else if(x=="MedOff") 
    {
     document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed Med";   
    }

      else if(x=="StopOff") 
    {
     document.getElementById("poolPumpSpeed").innerHTML = "Pump Stop";   
    }
     
   }

  function setTime(){
     var xhr = new XMLHttpRequest();
     const onTime = document.getElementById('ontime').value;
     const offTime = document.getElementById('offtime').value;
     
     
     console.log(onTime);
     console.log(offTime);
     
     if ((!isNaN(parseInt(onTime)))  && (!isNaN(parseInt(offTime)))){
      
     xhr.open("GET", "/timeAdjust?onTime="+onTime+"&offTime="+offTime, true);
     xhr.send();
    
    }
    else
    console.log("Error");
   }
   
   function changeOnTime(x) {
   console.log(x);
   document.getElementById("timeOn").innerHTML = x;
   }
   
   function changeOffTime(x) {
   console.log(x);
   document.getElementById("timeOff").innerHTML = x;
   }
   
   
   setInterval(function ( ) {
  console.log('Running interval');

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
  
    if (this.readyState == 4 && this.status == 200) {
     const obj = JSON.parse(this.responseText);
     var pumpSpeed;
     var RelaxStatus;
     var timeon;
     var timeoff;
     var rssi;
     var hh;
     var mm;
     var ss;
     var dd;
     var md;
     var yy;
     var temperatur;
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

document.getElementById("status_temp").innerHTML ="Water Temp: "+temperatur;

if (RelaxStatus==1) 
    {
      document.getElementById("poolRelaxState").innerHTML = "OK";
    }
    else if(RelaxStatus==0) 
    {
     document.getElementById("poolRelaxState").innerHTML = "FEJL";   
    }

if (pumpSpeed==0) 
    {
      document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed LOW";
    }
    else if(pumpSpeed==1) 
    {
     document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed HIGH";   
    }
     else if(pumpSpeed==2) 
    {
       document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed Not set"; 
    }
     else if(pumpSpeed==3) 
    {
       document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed Med"; 
    }
     else if(pumpSpeed==4) 
    {
       document.getElementById("poolPumpSpeed").innerHTML = "Pump Speed Stop"; 
    }

    document.getElementById("timeOn").innerHTML =timeon ;
    document.getElementById("timeOff").innerHTML =timeoff ; 
    document.getElementById("rssi").innerHTML ="RSSI "+rssi+" Time:"+hh+":"+mm+" Date "+dd+"."+md+"."+yy;


  }
 };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 2000 ) ;

setInterval(function ( ) {

var colorStatus = document.getElementById("mydot").style.backgroundColor;

if ((colorStatus == "red") && (server_running==true)) {
  document.getElementById("mydot").style.backgroundColor  = "green";
  server_running=false;
  }
  else
  {
  document.getElementById("mydot").style.backgroundColor  = "red";
  }



},1000);

  </script>
  </body>
</html>
)rawliteral";


const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <p>Logged out or <a href="/">return to homepage</a>.</p>
  <p><strong>Note:</strong> close all web browser tabs to complete the logout process.</p>
</body>
</html>
)rawliteral";
#ifndef WEB_H
#define WEB_H

#include <WebServer.h>
#include <HTTPUpdateServer.h>

class WebPage{
  public:
    WebPage(WebServer *server);
    void readMain();
    void readConfig();
    void updateField();
    void pressButton();
    void updateConfig();
    void setup();
  public:
    unsigned long lastUpdate = 0; // timestamp - last MQTT update
    unsigned long lastCallback = 0; // timestamp - last MQTT callback received
    unsigned long lastWiFiDisconnect=0;
    unsigned long lastWiFiConnect=0;
    unsigned long lastMQTTDisconnect=0; // last time MQTT was disconnected
    unsigned long WiFiLEDOn=0;
    unsigned long k1_up_pushed=0;
    unsigned long k1_down_pushed=0;

    String lastCommand = "";
    String crcStatus="";

  private:
    void Restart();
    int WifiGetRssiAsQuality(int rssi);
    void timeDiff(char *buf,size_t len,unsigned long lastUpdate);
  private:
    WebServer* _server;
};


/***************/
/*             */
/*   M A I N   */
/*             */
/***************/
//const char MAIN_page[] PROGMEM = R"#(
const char MAIN_page[] = R"#(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
.tilt {
  display: none;
}
.remote_control {
  display:grid;
  width: 40em;
  grid-template-columns: 1fr 1fr 10px 1fr 1fr;
  grid-template-rows:   2.5em 2.5em 3.5em;
  grid-template-areas: 
    "h1 h2 . h3 h4"
    "k1 k2 . k3 k4"
    "b1 b2 . b3 b4";
}
.status {
  display:grid;
  width: 20em;
  grid-template-columns: 6.3em 1fr 1fr 5px;
  grid-template-rows: 2.5em 2.5em 2.5em 2.5em;
}
.commands {
  display:grid;
  grid-template-columns: 160px 160px 160px 160px;
}
@media only screen and (max-width: 700px) {
  .remote_control {
    display:grid;
    width: 20em;
    grid-template-columns: 1fr 1fr;
    grid-template-rows:   2.5em 2.5em 3.5em 10px 2.5em 2.5em 3.5em;
    grid-template-areas: 
      "h1 h2"
      "k1 k2"
      "b1 b2"
      ". ."
      "h3 h4"
      "k3 k4"
      "b3 b4";    
  }
  .status {
    display:grid;
    width: 20em;
    grid-template-columns: 6.3em 1fr 1fr;
    grid-template-rows: 2.5em 2.5em 2.5em 2.5em;
  }
  .commands {
    display:grid;
    grid-template-columns: 1fr 1fr;
  }
}
html {
  box-sizing: border-box;
}
*, *:before, *:after {
  box-sizing: inherit;
}
body {
  padding: 20px; 
  background-color: #232323;
  font-family: Verdana, sans-serif; 
  font-size: 100%; 
  color: white;
}
input, label, div {
  display: flex;
  align-items: center;
  justify-content: center;
  height:100%;
  width: 100%;
  border: 1px solid grey;
  font-size: 1em;
}
.description { grid-column: 1/2; }
.s1 { grid-column: 2/3; }
.s2 { grid-column: 3/4; }
.h1 { grid-area: h1; }
.h2 { grid-area: h2; }
.h3 { grid-area: h3; }
.h4 { grid-area: h4; }
.k1 { grid-area: k1; }
.k2 { grid-area: k2; }
.k3 { grid-area: k3; }
.k4 { grid-area: k4; }
.b1 { grid-area: b1; }
.b2 { grid-area: b2; }
.b3 { grid-area: b3; }
.b4 { grid-area: b4; }
p {
  font-size: 0.875em;
}
h1, h2 { 
  font-family: "Bahnschrift Condensed", sans-serif; 
}
h1 { 
  color: #1fa2ec;
  font-size: 2em; 
}
h2 { 
  color:khaki;
  margin-top: 29px; 
  margin-bottom: 5px;
  font-size: 1.5 em;  
}
.topic { font-weight: bold; }
.button {
  background-color: #1fa3ec;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  margin: 5px;
  border: 0px;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
.reset{
  background-color: #d43535;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  margin: 5px;
  border: 0;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}

</style>

<script>
function pushButton(b) {
  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  request.open("GET", "pressButton?button="+b, true);
  request.send();
}

setInterval(function() {
  // Call a function repetatively with 0.5 Second interval
  readMain();
}, 500); //500mSeconds update rate

function enableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        return;
      }
  }
  css.insertRule(unique_title+' {display:none;}',0);
}
function disableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        css.deleteRule(i);
        return;
      }
  }
}

function readMain() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {        
      var resp= JSON.parse(this.responseText);
      document.title=resp.device;
      document.getElementById("device").innerHTML=resp.device;
      document.getElementById("mqttmsg").innerHTML=resp.mqttmsg;
      document.getElementById("disconnect").innerHTML=resp.disconnect;
      document.getElementById("crc").innerHTML=resp.crc;
      document.getElementById("mem").innerHTML=resp.mem;
      document.getElementById("wifi").innerHTML=resp.wifi;
      document.getElementById("strength").innerHTML=resp.strength+" %";
      document.getElementById("ip").innerHTML=resp.ip;
      document.getElementById("update").innerHTML=resp.update;
      for(i=1;i<=2;i++) {
        document.getElementById("key"+i).innerHTML = resp.keys[i-1];
        if(resp.keys[i-1]=='Pressed') {
          document.getElementById("key"+i).style.background = '#f8aaaa';          
          document.getElementById("key"+i).style.color= 'black';
        } else {
          document.getElementById("key"+i).style.background = '#76ec76';
          document.getElementById("key"+i).style.color = 'black';
        }
      }

      if (resp.tilting=="true")
        disableStyle(".tilt");
      else
        enableStyle(".tilt");
    }
  };
  xhttp.open("GET", "readMain", true);
  xhttp.send();
}
</script>


</head>
<html>
<body>
<header><h1 id="device"></h1></header>

<h2>Sensors</h2>
<section class="remote_control"> 
  <div class="h1">Shutter 1 UP</div>
  <div class="h2">Shutter 1 DOWN</div>
  <div class="k1" id="key1"></div>
  <div class="k2" id="key2"></div>
  <div class="b1"><button type="button" class="button" onmousedown="pushButton(1)" onmouseup="pushButton(11)">▲</button></div>
  <div class="b2"><button type="button" class="button" onmousedown="pushButton(2)" onmouseup="pushButton(12)">▼</button></div>
</section>

<h2>Shutters</h2>  
<section class="status">  
  <div class="description"></div>
  <div class="s1">Shutter 1</div>
  <div class="description">movement</div>
  <div id="movement1" class="s1"></div>
  <div class="description">position</div>
  <div id="position1" class="s1"></div>
  <div class="description tilt">Tilt</div>
  <div id="tilt1" class="s1 tilt"></div>
</section>

<h2>Connectivity</h2>
<section>
  <p><span class="topic">SSID:</span> <span id="wifi"></span></p>
  <p><span class="topic">Signal strenght:</span> <span id="strength"></span></p>
  <p><span class="topic">IP address:</span> <span id="ip"></span></p>
</section>

<h2>MQTT</h2>
<section>
  <p><span class="topic">Last received message:</span> <span id="mqttmsg"></span></p>
  <p><span class="topic">Last update:</span> <span id="update"></span></p>
  <p><span class="topic">Last loss of WiFi,MQTT:</span> <span id="disconnect"></span></p>
  <p><span class="topic">Boot CRC check:</span> <span id="crc"></span></p>
  <p><span class="topic">Free memory:</span> <span id="mem"></span></p>
</section>

<h2>Commands</h2>
<section class="commands">
  <button type="button" class="reset" onclick="location.href='/configure';">Configure</button>
  <button type="button" class="reset" onclick="location.href='/upgrade';">Upgrade</button>
  <button type="button" class="reset" onclick="location.href='/upload';">Upload TFT</button>
  <button type="button" class="reset" onmouseup="pushButton(55)">Calibrate</button>
  <button type="button" class="reset" onmouseup="pushButton(66)">Restart</button>
</section><br />
 
<footer><h6>Last code change: )#" __DATE__ " " __TIME__  R"#(</h6></footer>

</body>
</html>
)#";

/*************************/
/*                       */
/*   C O N F I G U R E   */
/*                       */
/*************************/
const char CONFIGURE_page[] PROGMEM = R"#(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
.tilt { 
  display: none;
}

.wifi_multi { 
  display: none;
}
.container {
  display:grid;
  grid-template-columns: 1em 25px 10em 6em 7em 6em 7em;
  grid-auto-rows: auto;
}
label.description,label.first,label.second,.checkbox {
  align-self: center;
}
.checkbox {
  grid-column: 2/3;
}
.description {
  grid-column: 3/4;
}
.first, .full {
  grid-column: 4/6;
}
.second { 
  grid-column: 6/8;
}
.header {
  grid-column: 2/-1;
}
.commands {
  display:grid;
  grid-template-columns: 160px 160px 160px 160px;
}
input, select {
  height: 2.2em;
}
label {
  height: 1.2em;
}
@media only screen and (max-width: 400px) {
  .container {
    grid-template-columns: 0px 22px 8em 1em 1fr 1em;
  }
  .full {
    grid-column: 4/6;
  }
  .first {
    grid-column: 4/6;
  }
  .second { 
    grid-column: 5/7;
  }
  .commands {
    grid-template-columns: 1fr 1fr;
  }
}
@media only screen and (min-width: 401px) and (max-width: 700px) {
  .container {
    grid-template-columns: 0px 22px 8em 1em 12em 1em;
  }
  .full {
    grid-column: 4/6;
  }
  .first {
    grid-column: 4/6;
  }
  .second { 
    grid-column: 5/7;
  }
  .commands {
    grid-template-columns: 1fr 1fr;
  }
}
html {
  box-sizing: border-box;
}
*, *:before, *:after {
  box-sizing: inherit;
}
body {
  padding: 20px; 
  background-color: #232323;
  font-family: Verdana, sans-serif; 
  font-size: 100%; 
  color: white;
} 
h1, h2 { 
  font-family: "Bahnschrift Condensed", sans-serif; 
}
h1 { 
  color: #1fa2ec;
  font-size: 2em; 
}
h2 { 
  color:khaki;
  margin-top: 29px; 
  margin-bottom: 5px;
  font-size: 1.5 em;  
}
.topic { font-weight: bold; }
.button {
  background-color: #1fa3ec;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  margin: 5px;
  border: 0;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
.reset{
  background-color: #d43535;
  border-radius: 0.3rem;
  transition-duration: 0.4s;
  cursor: pointer;
  border: 0;
  margin: 5px;
  color: white;
  padding: 7px 15px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
</style>

<script>
var BLIND_NUMBER=0;

function pushButton(b) {
  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  request.open("GET", "pressButton?button="+b, true);
  request.send();
  if(b==77)
    readConfig();
  if(b==89) {
    location.href='/';
  }
}

function enableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        return;
      }
  }
  css.insertRule(unique_title+' {display:none;}',0);
}
function disableStyle(unique_title) {
  var css=document.styleSheets[0];
  for(var i=0; i<css.cssRules.length; i++) {
    var rule = css.cssRules[i];
      if (css.cssRules[i].cssText.includes(unique_title)) {
        css.deleteRule(i);
        return;
      }
  }
}

function sendData(field,value, param) {
  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  
  if (field=="tilt") {
    if (value)
      disableStyle(".tilt");
    else
      enableStyle(".tilt");
  }

  request.open("GET", "updateField?field="+field+"&value="+value+"&param="+param, true);
  request.send();
}

function sendConfig()
{

  var data={
      "host_name":"",
      "wifi_ssid1":"",
      "wifi_password1":"",
      "mqtt_server":"",
      "blinds":[]};

  data.host_name= document.getElementById("host_name").value;
  data.wifi_ssid1= document.getElementById("wifi_ssid1").value;
  data.wifi_password1= document.getElementById("wifi_password1").value;
  data.mqtt_server= document.getElementById("mqtt_server").value;

 for(var i=0; i<BLIND_NUMBER;i++)
 {
  b_name_string=i+1;
  data.blinds[i]= document.getElementById("blind_name_"+b_name_string).value;
 }

  var request = new XMLHttpRequest();
  request.onreadystatechange = function() { 
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  request.open("POST", "updateConfig");
  request.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
  request.send(JSON.stringify(data));
}

function myFunction(index){
  sendData("blind_names",this.value,index)
}

function readConfig() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var resp= JSON.parse(this.responseText);
      document.getElementById("host_name").value = resp.host_name;
      document.getElementById("wifi_ssid1").value = resp.wifi_ssid1;
      document.getElementById("wifi_password1").value = resp.wifi_password1;
      document.getElementById("mqtt_server").value = resp.mqtt_server;
      BLIND_NUMBER=0;
      for (i=0; i<resp.blind_names.length;i++){
        var b_name_string=i+1;
        BLIND_NUMBER=BLIND_NUMBER+1;
        if( document.getElementById("blind_name_"+b_name_string)==null){
         var lbl = document.createElement("label");
         lbl.className="description";
         lbl.setAttribute("for","blind_name_"+b_name_string);
         lbl.innerHTML = "Blind "+b_name_string;
         var input = document.createElement("input");
         input.type = "text";
         input.name = "blind_name_" + b_name_string;
         input.id = "blind_name_" + b_name_string;
         input.maxlength="24";
         input.className="full";
         var container = document.getElementById("name_container");
         container.appendChild(lbl);
         container.appendChild(input);
         } 
         document.getElementById("blind_name_"+b_name_string).value = resp.blind_names[i];
      }            
    }
  };  
  xhttp.open("GET", "readConfig", true);
  xhttp.send();
}
</script>


</head>
<html>
<body onload="readConfig();">
<header><h1 class="header" id="device">Configuration</h1></header>

<section class="container">
  <label class="description" for="host_name">Host name</label> <input class="full" type="text" name="host_name" id="host_name" onchange="sendData(this.id,this.value);">
</section>

<h2>WiFi</h2>
<section class="container">
  <label class="description" for="wifi_ssid1">SSID 1</label> <input class="full" type="text" maxlength="24" name="wifi_ssid1" id="wifi_ssid1" onchange="sendData(this.id,this.value);">
  <label class="description" for="wifi_password1">password 1</label> <input class="full" type="text" maxlength="24" name="wifi_password1" id="wifi_password1" onchange="sendData(this.id,this.value);">
</section>

<h2>MQTT</h2>
<section class="container">
  <label class="description" for="mqtt_server">Server</label> <input class="full" type="text" maxlength="24" name="mqtt_server" id="mqtt_server">
</section> 

<h2>Names</h2>
<section class="container" id="name_container">
</section>  

<section class="commands">
<button type="button" class="reset" onclick="location.href='/';">Back</button>
<button type="button" class="reset" onmouseup="pushButton(77)">Load defaults</button>
<button type="button" class="reset" onmouseup="pushButton(88)">Save and restart</button>
<button type="button" class="reset" onmouseup="sendConfig()">Save Configuration</button>
</section>
</body>
</html>
)#";

/*******************************************
*
********************************************/
const char TFTUPLOAD_page[] PROGMEM = R"#(
<!DOCTYPE html>
<html>
<head>
  <meta http-equiv="Content-type" content="text/html; charset=utf-8">
  <title>Nextion updater</title>
</head>
<body>
    <h3>Choose .tft file to upload</h3>
    <form method="post" action="" enctype="multipart/form-data">
        <input type="file" name="name" onchange="valSize(this)">
        <input id="button" type="submit" value="Upload & Update" disabled>
    </form>
    <script>
      function valSize(file){
        // get the selected file size and send it to the ESP
        var fs = file.files[0].size;
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function(){
          if(this.readyState == 4 && this.status == 200){
            // ESP received fileSize enable the submit button
            document.getElementById("button").disabled = false;
          }
        };
        xhttp.open("POST", "/fs", true);
        xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        xhttp.send("fileSize="+fs);
      }
    </script>
    <p>Updating might take a while if you have complex .tft file. Check Nextion display for progress.</p>
</body>
</html>
  )#";

/*********************************
* SUCCESS
******************************/
const char SUCCESS_page[] PROGMEM = R"#(
  <!DOCTYPE html>
	<html lang="en">
	<head>
		<title>Nextion Updater</title>
	</head>
	<body>
		Update successful
	</body>
</html>
)#";

/********************************
*FAILURE
**********************************/
const char FAILURE_page[] PROGMEM = R"#(
<!DOCTYPE html>
	<html lang="en">
	<head>
		<title>Nextion Updater</title>
		<script>
			function getUrlVars() {
				var vars = {};
				var parts = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function(m,key,value) {
				vars[key] = value;
				});
				return vars;
			}

			function setReason() {
				var reason = getUrlVars()["reason"];
				document.getElementById("reason").innerHTML = decodeURIComponent(reason);
			}
		</script>
	</head>
	<body onLoad="setReason()">
		<h2>Update failed</h2>
		Reason: <strong><span id="reason"></span></strong>
	</body>
</html>
)#";

#endif
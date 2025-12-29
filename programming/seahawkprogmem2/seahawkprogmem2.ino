#include <SPI.h>
#include <Ethernet.h>

#ifndef FPSTR
  #define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#endif

// -------------------- Turbidity Sensor --------------------
const int turbidityPin = A3;   // Analog pin for turbidity sensor
float readNTU();               // Forward declaration

// -------------------- LED --------------------
const int LED_PIN = 8;         // LED on pin 8
bool ledState = false;         // track LED ON/OFF state

// -------------------- Network Settings --------------------
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);  // adjust to your LAN
EthernetServer server(80);

// -------------------- Motor Pins --------------------
// right Motor
int ENA = 5;
int IN1 = 22;
int IN2 = 23;

// left Motor
int ENB = 6;
int IN3 = 25;
int IN4 = 24;

// Vertical Motor
int ENC = 7;  // PWM
int IN5 = 26;
int IN6 = 27;

// -------------------- Motor Variables --------------------
int currentSpeed = 255;  // default PWM

// -------------------- HTML Page --------------------
const char controlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
<title>SEAHAWK Control</title>
<style>
body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #001f3f, #0074D9); color: white; text-align: center; margin: 0; padding: 0; }
h1 { margin-top: 20px; font-size: 28px; }
p { font-size: 16px; margin-bottom: 10px; }
#status { margin: 20px; font-size: 20px; font-weight: bold; color: #ff3333; }
.controls { display: grid; grid-template-columns: 100px 100px 100px; grid-gap: 15px; justify-content: center; margin-top: 30px; }
button { background: #111; color: #fff; border: 2px solid #0ff; border-radius: 12px; padding: 15px; font-size: 20px; cursor: pointer; transition: 0.2s; }
button:hover { background: #0ff; color: #111; transform: scale(1.1); }
.stop-btn { background: #b30000; border: 2px solid #ff4d4d; font-weight: bold; font-size: 22px; }
.stop-btn:hover { background: #ff1a1a; color: #fff; transform: scale(1.2); box-shadow: 0 0 15px #ff4d4d; }
.active { background: #0ff !important; color: #111 !important; box-shadow: 0 0 20px #0ff; }
.stop-active { background: #ff1a1a !important; color: #fff !important; box-shadow: 0 0 20px #ff4d4d; }

/* Turbidity display box on the right side */
#turbidityBox {
  position: fixed;
  top: 20px;
  right: 20px;
  background: rgba(0, 0, 0, 0.5);
  border: 2px solid #0ff;
  border-radius: 12px;
  padding: 12px 18px;
  font-size: 16px;
  box-shadow: 0 0 10px #0ff;
  text-align: left;
}
#turbidityLabel {
  font-weight: bold;
}
#turbidityValue {
  font-size: 20px;
  color: #0ff;
}

/* LED ON visual effect */
.led-on {
  box-shadow: 0 0 18px #0f0;
  border-color: #0f0;
}
</style>
</head>
<body>
<h1>SEAHAWK  Control Panel</h1>
<p>W = Forward | S = Backward | A = Left | D = Right</p>
<p>U = Up | J = Down | X = STOP | L = LED TOGGLE</p>
<div id='status'>Current Action: STOPPED</div>

<!-- Turbidity live display -->
<div id="turbidityBox">
  <div id="turbidityLabel">Turbidity of Water:</div>
  <div><span id="turbidityValue">--.-</span> NTU</div>
</div>

<div class='controls'>
  <div></div><button id='btnW' onclick="sendCommand('W')">W</button><div></div>
  <button id='btnA' onclick="sendCommand('A')">A</button>
  <button id='btnX' class='stop-btn' onclick="sendCommand('X')">X</button>
  <button id='btnD' onclick="sendCommand('D')">D</button>
  <div></div><button id='btnS' onclick="sendCommand('S')">S</button><div></div>
</div>

<div style="margin-top:40px;">
  <button id='btnU' onclick="sendCommand('U')">U (Up)</button>
  <button id='btnJ' onclick="sendCommand('J')">J (Down)</button>
</div>

<div style="margin-top:40px;">
  <label for="speedSlider">Speed: <span id="speedValue">255</span></label><br>
  <input type="range" min="0" max="255" value="255" id="speedSlider" style="width:200px;" oninput="updateSpeed(this.value)">
  <div style="margin-top:10px%;">
    <button onclick="changeSpeed(10)">faster</button>
    <button onclick="changeSpeed(-10)">slower</button>
  </div>
</div>

<!-- LED TOGGLE CONTROL (ADDED) -->
<div style="margin-top:40px;">
  <h2>LED Control</h2>
  <button id="btnLed" onclick="toggleLed()">LED OFF (L)</button>
</div>

<script>
let currentSpeed = 255;
let ledState = false;   // track LED state in UI

function setStatus(cmd) {
  let action = ''; let color = '';
  switch(cmd) {
    case 'W': action = 'FORWARD'; color = 'lime'; break;
    case 'S': action = 'BACKWARD'; color = 'lime'; break;
    case 'A': action = 'LEFT'; color = 'lime'; break;
    case 'D': action = 'RIGHT'; color = 'lime'; break;
    case 'U': action = 'UP'; color = 'lime'; break;
    case 'J': action = 'DOWN'; color = 'lime'; break;
    case 'X': action = 'STOPPED'; color = 'red'; break;
  }
  let statusBox = document.getElementById('status');
  statusBox.innerText = 'Current Action: ' + action;
  statusBox.style.color = color;
}

function activateButton(cmd) {
  let btn = document.getElementById('btn' + cmd);
  if(btn){
    if(cmd === 'X'){ btn.classList.add('stop-active'); }
    else { btn.classList.add('active'); }
  }
}

function deactivateButton(cmd) {
  let btn = document.getElementById('btn' + cmd);
  if(btn){ btn.classList.remove('active'); btn.classList.remove('stop-active'); }
}

function sendCommand(cmd) {
  fetch('/cmd?c=' + cmd);
  setStatus(cmd);
}

function updateSpeed(val){
  currentSpeed = val;
  document.getElementById('speedValue').innerText = val;
  fetch('/speed?s=' + val);
}

function changeSpeed(delta){
  let newSpeed = currentSpeed + delta;
  if(newSpeed > 255) newSpeed = 255;
  if(newSpeed < 0) newSpeed = 0;
  currentSpeed = newSpeed;
  // automatically update slider & value
  document.getElementById('speedSlider').value = newSpeed;
  document.getElementById('speedValue').innerText = newSpeed;
  fetch('/speed?s=' + newSpeed);
}

// LED TOGGLE (ADDED)
function toggleLed(){
  ledState = !ledState;
  const state = ledState ? 'on' : 'off';
  fetch('/led?state=' + state);

  const btn = document.getElementById('btnLed');
  if(ledState){
    btn.innerText = 'LED ON (L)';
    btn.classList.add('led-on');
  } else {
    btn.innerText = 'LED OFF (L)';
    btn.classList.remove('led-on');
  }
}

// Keyboard controls
document.addEventListener('keydown', function(e) {
  var key = e.key.toUpperCase();
  if(['W','A','S','D','U','J','X'].includes(key)) {
    activateButton(key);
    sendCommand(key);
  }
  if(e.key === 'ArrowUp') changeSpeed(10);
  if(e.key === 'ArrowDown') changeSpeed(-10);

  // L = LED toggle (ADDED)
  if(key === 'L') {
    toggleLed();
  }
});

document.addEventListener('keyup', function(e) {
  var key = e.key.toUpperCase();
  if(['W','A','S','D','U','J','X'].includes(key)) {
    deactivateButton(key);
    if(key !== 'X'){ sendCommand('X'); }
  }
});

// ---- Live turbidity polling ----
function fetchTurbidity() {
  fetch('/turbidity')
    .then(response => response.text())
    .then(text => {
      document.getElementById('turbidityValue').innerText = text.trim();
    })
    .catch(err => {
      console.log('Error reading turbidity:', err);
    });
}

// Poll every 1 second
setInterval(fetchTurbidity, 1000);
fetchTurbidity(); // initial call
</script>
</body>
</html>
)rawliteral";

// -------------------- Setup --------------------
void setup() {
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENC, OUTPUT); pinMode(IN5, OUTPUT); pinMode(IN6, OUTPUT);

  // LED setup (ADDED)
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // start OFF

  Ethernet.begin(mac, ip);
  server.begin();

  Serial.begin(9600);
  Serial.print("Open in browser: http://");
  Serial.println(Ethernet.localIP());
}

// -------------------- Loop --------------------
void loop() {
  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    String request = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        if (c == '\n' && currentLineIsBlank) {
          // --- Handle Commands ---
          if(request.indexOf("GET /cmd?c=") >= 0){
            if (request.indexOf("c=W") >= 0) forward();
            else if (request.indexOf("c=S") >= 0) backward();
            else if (request.indexOf("c=A") >= 0) turnLeft();
            else if (request.indexOf("c=D") >= 0) turnRight();
            else if (request.indexOf("c=U") >= 0) up();
            else if (request.indexOf("c=J") >= 0) down();
            else if (request.indexOf("c=X") >= 0) stopAll();

            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-Type: text/plain"));
            client.println(F("Connection: close"));
            client.println();
            client.println("OK");
          }
          // --- Handle Speed ---
          else if(request.indexOf("GET /speed?s=") >= 0){
            int eqPos = request.indexOf("=");
            if(eqPos > 0){
              String valStr = request.substring(eqPos + 1, request.indexOf(" ", eqPos));
              currentSpeed = valStr.toInt();
              Serial.print("Speed updated: "); Serial.println(currentSpeed);
            }
            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-Type: text/plain"));
            client.println(F("Connection: close"));
            client.println();
            client.println("OK");
          }
          // --- Handle LED (ADDED) ---
          else if(request.indexOf("GET /led?state=") >= 0){
            if(request.indexOf("state=on") >= 0){
              digitalWrite(LED_PIN, HIGH);
              ledState = true;
            } else if(request.indexOf("state=off") >= 0){
              digitalWrite(LED_PIN, LOW);
              ledState = false;
            }
            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-Type: text/plain"));
            client.println(F("Connection: close"));
            client.println();
            client.println("OK");
          }
          // --- Handle Turbidity ---
          else if(request.indexOf("GET /turbidity") >= 0){
            float ntu = readNTU();
            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-Type: text/plain"));
            client.println(F("Connection: close"));
            client.println();
            client.println(ntu, 1);   // one decimal place
          }
          // --- Serve HTML Page ---
          else {
            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-Type: text/html"));
            client.println(F("Connection: close"));
            client.println();
            client.print(FPSTR(controlPage));
          }
          break;
        }
        if(c == '\n') currentLineIsBlank = true;
        else if(c != '\r') currentLineIsBlank = false;
      }
    }
    delay(1);
    client.stop();
  }
}

// -------------------- Turbidity Function --------------------
float readNTU() {
  const int samples = 10;
  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(turbidityPin);
    delay(10);
  }

  float avgValue = sum / (float)samples;
  float voltage = avgValue * (5.0 / 1023.0);

  // Example mapping (CALIBRATE FOR YOUR SENSOR)
  float ntu = -112.0 * voltage * voltage + 574.2 * voltage - 435.0;
  if (ntu < 0) ntu = 0;   // avoid negative

  Serial.print("Turbidity ADC: ");
  Serial.print(avgValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage);
  Serial.print(" V | NTU: ");
  Serial.println(ntu);

  return ntu;
}

// -------------------- Motor Functions --------------------
void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, currentSpeed);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, currentSpeed);
}

void backward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, currentSpeed);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); analogWrite(ENB, currentSpeed);
}

void up() {
  digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW); analogWrite(ENC, currentSpeed);
}

void down() {
  digitalWrite(IN5, LOW); digitalWrite(IN6, HIGH); analogWrite(ENC, currentSpeed);
}

void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, currentSpeed);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); analogWrite(ENB, currentSpeed);
}

void turnLeft() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, currentSpeed);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, currentSpeed);
}

void stopAll() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); analogWrite(ENA, 0);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); analogWrite(ENB, 0);
  digitalWrite(IN5, LOW); digitalWrite(IN6, LOW); analogWrite(ENC, 0);
}

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

const char* ssid = "OpenWrt";
const char* password = "La1003000902";
WebServer server(80);


const int motorPin = 11; // Change this to the pin number connected to the motor relay
const int sensorPin = 16; // Change this to the pin number connected to the moisture sensor


//storage
void writeLongToEEPROM(int address, long value) {
  // write the four bytes of the long integer to EEPROM
  EEPROM.write(address, (value >> 24) & 0xFF);
  EEPROM.write(address + 1, (value >> 16) & 0xFF);
  EEPROM.write(address + 2, (value >> 8) & 0xFF);
  EEPROM.write(address + 3, value & 0xFF);
  EEPROM.commit();
}
long readLongFromEEPROM(int address) {
  // read the four bytes of the long integer from EEPROM
  byte b1 = EEPROM.read(address);
  byte b2 = EEPROM.read(address + 1);
  byte b3 = EEPROM.read(address + 2);
  byte b4 = EEPROM.read(address + 3);
  // combine the four bytes to form a long integer
  long value = ((long)b1 << 24) | ((long)b2 << 16) | ((long)b3 << 8) | (long)b4;
  return value;
}



//moisture
const int maxAnalogValue = 80000; 
const int numReadings = 10;  // Number of readings to average
int readings[numReadings];   // Array to store readings
int readingIndex = 0;        // Current index in array
int total = 0;              // Running total of readings from history
int moisture = 0;           //current calculated moisture level
int getMoisture(){
  //motor is messing with readings
  bool mot = getMotor();
  setMotor(true);
  delay(10);
  // Read the sensor value and add it to the total
  total -= readings[readingIndex];
  readings[readingIndex] = analogRead(sensorPin);
  total += readings[readingIndex];
  // Move to the next index in the array
  readingIndex = (readingIndex + 1) % numReadings;
  // Calculate the average value
  moisture = total / numReadings;
  setMotor(mot);
  return moisture;
}
int getMoisturePercent(){
  int m = getMoisture();
  //TODO:fix percentage calculation
  m = map(m,0,8000,0,100);
  return m;
}


//desired moisture
int desiredMoisture; // The desired moisture level
const int desiredMoistureAddress = 0; // Address to store the desired moisture level in EEPROM
void setDesiredMoisture(int m){
  if (abs(m - desiredMoisture) >= 50) {
      desiredMoisture = m;
      writeLongToEEPROM(desiredMoistureAddress, desiredMoisture);
      
    }
}

void setDesiredMoisturePercent(int m){
  //todo map from percent
  setDesiredMoisture(m);
}

int getDesiredMoisture(){
  desiredMoisture = readLongFromEEPROM(desiredMoistureAddress);
  return desiredMoisture;
}

int getDesiredMoisturePercent(){
  int m = getDesiredMoisturePercent();
  //todo map
  return m;
}


//motor
bool motorSpinning = false;
void setMotor(bool state){
  state = state; //add a ! if need to invert command
  digitalWrite(motorPin, state);
  motorSpinning = state;
}
bool getMotor(){
  return motorSpinning;
}


//trigger watering
void waterTrigger(){
  // Add some wiggle room to the desired moisture level
  int d = getDesiredMoisture();
  int m = getMoisture();
  bool mot = getMotor();
  int threshold = d * 0.1;
  int lowerThreshold = d - threshold;
  int upperThreshold = d + threshold;
  
  // Check if the moisture level is below the lower threshold
  if (m < lowerThreshold) {
    if (!mot){    
      setMotor(true); 
    }
  }
  // Check if the moisture level is above the upper threshold
  else if (m > upperThreshold) {
    if (mot){    
      setMotor(false); 
    }
  }
}


//Webinterface
void webserver() {
  String html = "<!DOCTYPE html><html><head><title>Moisture Control</title>";
  
  //script
  html += "<script>";
  
  html += "function updateMoisture() {";
  html += "var xhr = new XMLHttpRequest();";
  html += "xhr.onreadystatechange = function() {";
  html +=   "if (xhr.readyState === 4 && xhr.status === 200) {";
  html +=     "document.getElementById('moisture').textContent = xhr.responseText;";
  html +=     "}";
  html +=   "};";
  html +=   "xhr.open('GET', '/moisture', true);";
  html +=   "xhr.send(null);";
  html += "}";
  html += "setInterval(updateMoisture, 1000);";

  html += "function updateMotorstate() {";
  html += "var xhr = new XMLHttpRequest();";
  html += "xhr.onreadystatechange = function() {";
  html +=   "if (xhr.readyState === 4 && xhr.status === 200) {";
  html +=     "document.getElementById('pumps-running').textContent = xhr.responseText;";
  html +=     "}";
  html +=   "};";
  html +=   "xhr.open('GET', '/pump', true);";
  html +=   "xhr.send(null);";
  html += "}";
  html += "setInterval(updateMotorstate, 1000);";

  html += "function setMotor(state) {";
  html += "var xhr = new XMLHttpRequest();";
  html += "xhr.onreadystatechange = function() {";
  html +=   "if (xhr.readyState === 4 && xhr.status === 200) {";
  html +=     "document.getElementById('moisture').textContent = xhr.responseText;";
  html +=     "}";
  html +=   "};";
  html +=   "xhr.open('POST', '/pump', true);";
  html +=   "xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
  html +=   "xhr.send('set=' + state);";
  html += "}";

  html += "function setDesiredMoisture() {";
  html += "var xhr = new XMLHttpRequest();";
  html += "xhr.onreadystatechange = function() {";
  html +=   "if (xhr.readyState === 4 && xhr.status === 200) {";
  html +=       "document.getElementById('desired-moisture').textContent = xhr.responseText;";
  html +=       "}";
  html +=     "};";
  html +=   "var desiredMoisture = document.getElementById('desiredMoisture').value;";
  html +=   "xhr.open('POST', '/desired-moisture', true);";
  html +=   "xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
  html +=   "xhr.send('desiredMoisture=' + desiredMoisture);";
  html += "}";
  
  html += "</script>";

  //html
  html += "</head><body>";
  html += "<h1>Moisture Control</h1>";
  html += "<p>Moisture: <span id='moisture'>"+String( getMoisture())+"</span></p>";
  html += "<p>Desired Moisture: <span id='desired-moisture'>"+String( getDesiredMoisture())+"</span></p>";
  html += "<p>Pumps running: <span id='pumps-running'>"+String(getMotor())+"</span></p>";
  html += "<button onclick='setMotor(1)'>Turn pump on</button>";
  html += "<button onclick='setMotor(0)'>Turn pump off</button>";
  html += "<p>Set new target for desired moisture:";
  html += "<input type='number' id='desiredMoisture' value='" + String( getDesiredMoisture()) + "'><br>";
  html += "<button onclick='setDesiredMoisture()'>Set</button>";
  html += "</body></html>";

  //style
  html += "<style>";
  html += "</style>";
  
  server.send(200, "text/html", html);
}

void webserverMoisture() {
    String response = String(moisture);
    server.send(200, "text/plain", response);
}

void webserverDesiredMoisture() {
  if (server.hasArg("desiredMoisture")) {
    int newDesiredMoisture = server.arg("desiredMoisture").toInt();
    setDesiredMoisture(newDesiredMoisture);
    String response = String(getDesiredMoisture());
    server.send(200, "text/plain", response);
  } else {
    String response = String(getDesiredMoisture());
    server.send(200, "text/plain", response);
  }
}

void webserverPump() {
  if (server.hasArg("set")) {
    int setMotorsWeb = server.arg("set").toInt();
    //TODO: add write pump status 
    if (setMotorsWeb == 1){
      setMotor(true);
    }else if (setMotorsWeb == 0){
      setMotor(false);
    }
    server.send(200, "text/plain", "OK");
  } else {
    String response = String(getMotor());
    server.send(200, "text/plain", response);
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  //not really needed 
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  
  pinMode(sensorPin, INPUT);



  // Read the desired moisture level from EEPROM address 0
  EEPROM.begin(128);
  desiredMoisture = readLongFromEEPROM(0);


  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", webserver);
  server.on("/moisture", webserverMoisture);
  server.on("/desired-moisture", webserverDesiredMoisture);
  server.on("/pump", webserverPump);
  server.begin();
}

unsigned long previousMillis = 0;
const long interval = 1000; // interval in milliseconds
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    waterTrigger();
  }
  server.handleClient();
}

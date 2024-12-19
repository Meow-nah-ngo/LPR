#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>
// WiFi credentials and server information
const char* ssid = "jaruwan";
const char* password = "6644220000";
String aiApiUrl = "https://4cfd-2001-fb1-1d-272f-386d-8aff-d78f-6d15.ngrok-free.app";
String aiApiKey = "2oWm50vSL5hBzeDQEr2zdIphsXo_2MpnEtccn6hXU81purYjt";
String serverPath = "/process_image";
const int serverPort = 80;
const String spreadsheetId = "fabled-progress-445115-h7";
const String googleAccessToken = "https://oauth2.googleapis.com/token";
const String googleSheetsRange = "Sheet1!A2";
const String googleSheetsURL = "https://sheets.googleapis.com/v4/spreadsheets/" + spreadsheetId + "/values/" + googleSheetsRange + ":append?valueInputOption=RAW";
#define flashLight 4 // GPIO pin for the flashlight
int count = 0; // Counter for image uploads
WiFiClientSecure client; // Secure client for HTTPS communication
// Camera GPIO pins - adjust based on your ESP32-CAM board
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
// Network Time Protocol (NTP) setup
const char* ntpServer = "pool.ntp.org"; // NTP server
const long utcOffsetInSeconds = 19800; // IST offset (UTC + 5:30)
int servoPin = 14; // GPIO pin for the servo motor
int inSensor = 13; // GPIO pin for the entry sensor
int outSensor = 15; // GPIO pin for the exit sensor
Servo myservo; // Servo object
int pos = 0; // Variable to hold servo position
// Initialize the NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds);
String currentTime = "";
// Web server on port 80
WebServer server(80);
// Variables to hold recognized data, current status, and history
String recognizedPlate = ""; // Variable to store the recognized plate number
String imageLink = ""; // Variable to store the image link
String currentStatus = "Idle"; // Variable to store the current status of the system
int availableSpaces = 4; // Total parking spaces available
int vehicalCount = 0; // Number of vehicles currently parked
int barrierDelay = 3000; // Delay for barrier operations
int siteRefreshTime = 1; // Web page refresh time in seconds
// History of valid number plates and their entry times
struct PlateEntry {
  String plateNumber; // Plate number of the vehicle
  String time; // Entry time of the vehicle
};
std::vector<PlateEntry> plateHistory; // Vector to store the history of valid plates
// Function to extract a JSON string value by key
String extractJsonStringValue(const String& jsonString, const String& key) {
  int keyIndex = jsonString.indexOf(key);
  if (keyIndex == -1) {
    return "";
  }
  int startIndex = jsonString.indexOf(':', keyIndex) + 2;
  int endIndex = jsonString.indexOf('"', startIndex);
  if (startIndex == -1 || endIndex == -1) {
    return "";
  }
  return jsonString.substring(startIndex, endIndex);
}
void sendToArduinoAPI(String data) {
  HTTPClient http;
  String apiURL = "https://your-arduino-api-endpoint";
  http.begin(apiURL); // เริ่มการเชื่อมต่อกับ API
  http.addHeader("Content-Type", "application/json");
  String jsonData = "{\"data\":\"" + data + "\"}";
  int httpResponseCode = http.POST(jsonData);
  if (httpResponseCode > 0) {
    Serial.println("Data sent successfully to Arduino API");
  } else {
    Serial.println("Error in sending data to Arduino API");
  }
  http.end();
}
void sendToGoogleSheets(String plateNumber, String timestamp) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(googleSheetsURL);
    // Add headers
    http.addHeader("Authorization", "Bearer " + googleAccessToken);
    http.addHeader("Content-Type", "application/json");
    // Prepare JSON payload
    String jsonData = "{\"values\": [[\"" + plateNumber + "\", \"" + timestamp + "\"]]}";
    // Send POST request
    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Google Sheets Response: " + response);
    } else {
      Serial.println("Error sending to Google Sheets: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected!");
  }
}
// Function to send data to AI API
void sendToAIAPI(String imageLink) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // รวม aiApiUrl กับ serverPath เพื่อให้ได้ URL เต็ม
    String fullApiUrl = aiApiUrl + serverPath; // aiApiUrl คือ "https://4cfd-2001-fb1-1d-272f-386d-8aff-d78f-6d15.ngrok-free.app" และ serverPath คือ "/process_image"
    http.begin(fullApiUrl); // เรียกใช้ URL ที่รวม path แล้ว
    // เพิ่ม headers
    http.addHeader("Authorization", "Bearer " + aiApiKey); // ต้องกำหนด aiApiKey
    http.addHeader("Content-Type", "application/json");
    // เตรียม JSON payload
    String jsonData = "{\"image_url\": \"" + imageLink + "\"}";
    // ส่ง POST request
    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("AI API Response: " + response);
    } else {
      Serial.println("Error sending to AI API: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected!");
  }
}
void handleTrigger() {
  currentStatus = "Capturing Image";
  server.handleClient();
  // server.sendHeader("Location", "/"); // Redirect to root to refresh status
  // server.send(303); // Send redirect response to refresh the page
  // Perform the image capture and upload
  int status = sendPhoto();
  // Update status based on sendPhoto result
  if (status == -1) {
    currentStatus = "Image Capture Failed";
  } else if (status == -2) {
    currentStatus = "Server Connection Failed";
  } else if (status == 1) {
    currentStatus = "No Parking Space Available";
  } else if (status == 2) {
    currentStatus = "Invalid Plate Recognized [No Entry]";
  } else if (status == 0) {
    sendToGoogleSheets(recognizedPlate, currentTime); // ส่งไปยัง Google Sheets
    sendToAIAPI(imageLink); // ส่งไปยัง AI API
    sendToArduinoAPI(recognizedPlate); // ส่งข้อมูลไป Arduino API
  } else {
    currentStatus = "Idle";
  }
  server.handleClient(); // Update status on webpage
}
void openBarrier() {
  currentStatus = "Barrier Opening";
  server.handleClient(); // Update status on webpage
  Serial.println("Barrier Opens");
  myservo.write(0);
  delay(barrierDelay);
}
void closeBarrier() {
  currentStatus = "Barrier Closing";
  server.handleClient(); // Update status on webpage
  Serial.println("Barrier Closes");
  myservo.write(180);
  delay(barrierDelay);
}
// Function to capture and send photo to the server
int sendPhoto() {
  camera_fb_t* fb = NULL;
  // Turn on flashlight and capture image
  digitalWrite(flashLight, HIGH);
  delay(300);
  fb = esp_camera_fb_get();
  delay(300);
  digitalWrite(flashLight, LOW);
  if (!fb) {
    Serial.println("Camera capture failed");
    currentStatus = "Image Capture Failed";
    server.handleClient(); // Update status on webpage
    return -1;
  }
  // Connect to server
  Serial.println("Connecting to server:" + serverName);
  client.setInsecure(); // Skip certificate validation for simplicity
  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");
    // Increment count and prepare file name
    count++;
    Serial.println(count);
    String filename = apiKey + ".jpeg";
    // Prepare HTTP POST request
    String head = "--CircuitDigest\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"" + filename + "\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--CircuitDigest--\r\n";
    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=CircuitDigest");
    client.println("Authorization:" + apiKey);
    client.println();
    client.print(head);
    // Send the image
    currentStatus = "Uploading Image";
    server.handleClient(); // Update status on webpage
    // Send image data in chunks
    uint8_t* fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n += 1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    client.print(tail);
    // Release the frame buffer
    esp_camera_fb_return(fb);
    Serial.println("Image sent successfully");
    // Waiting for server response
    currentStatus = "Waiting for Server Response";
    server.handleClient(); // Update status on webpage
    String response = "";
    long startTime = millis();
    while (client.connected() && millis() - startTime < 10000) {
      if (client.available()) {
        char c = client.read();
        response += c;
      }
    }
    // Extract data from response
    recognizedPlate = extractJsonStringValue(response, "\"number_plate\"");
    imageLink = extractJsonStringValue(response, "\"view_image\"");
    currentStatus = "Response Recieved Successfully";
    server.handleClient(); // Update status on webpage
    // Add valid plate to history
    if (vehicalCount > availableSpaces) {
      // Log response and return
      Serial.print("Response: ");
      Serial.println(response);
      client.stop();
      esp_camera_fb_return(fb);
      return 1;
    } else if (recognizedPlate.length() > 4 && recognizedPlate.length() < 11) {
      // Valid plate
      PlateEntry newEntry;
      newEntry.plateNumber = recognizedPlate + "-Entry";
      newEntry.time = currentTime; // Use the current timestamp
      plateHistory.push_back(newEntry);
      vehicalCount++;
      openBarrier();
      delay(barrierDelay);
      closeBarrier();
      // Log response and return
      Serial.print("Response: ");
      Serial.println(response);
      client.stop();
      esp_camera_fb_return(fb);
      return 0;
    } else {
      currentStatus = "Invalid Plate Recognized '" + recognizedPlate + "' " + "[No Entry]";
      server.handleClient(); // Update status on webpage
      // Log response and return
      Serial.print("Response: ");
      Serial.println(response);
      client.stop();
      esp_camera_fb_return(fb);
      return 2;
    }
  } else {
    Serial.println("Connection to server failed");
    esp_camera_fb_return(fb);
    return -2;
  }
}
void setup() {
  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  pinMode(flashLight, OUTPUT);
  pinMode(inSensor, INPUT_PULLUP);
  pinMode(outSensor, INPUT_PULLUP);
  digitalWrite(flashLight, LOW);
  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  // Initialize NTPClient
  timeClient.begin();
  timeClient.update();
  // Start the web server
  server.on("/", handleRoot);
  server.on("/trigger", HTTP_POST, handleTrigger);
  server.begin();
  Serial.println("Web server started");
  // Configure camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // Adjust frame size and quality based on PSRAM availability
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 5; // Lower number means higher quality (0-63)
    config.fb_count = 2;
    Serial.println("PSRAM found");
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12; // Lower number means higher quality (0-63)
    config.fb_count = 1;
  }
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50); // standard 50 hz servo
  myservo.attach(servoPin, 1000, 2000); // attaches the servo on pin 18 to the servo object
    // Set the initial position of the servo (barrier closed)
  myservo.write(180);
}
void loop() {
  // Update the NTP client to get the current time
  timeClient.update();
  currentTime = timeClient.getFormattedTime();
  // Check the web server for any incoming client requests
  server.handleClient();
  // Monitor sensor states for vehicle entry/exit
  if (digitalRead(inSensor) == LOW && vehicalCount < availableSpaces) {
    delay(2000); // delay for vehicle need to be in a position
    handleTrigger(); // Trigger image capture for entry
  }
  if (digitalRead(outSensor) == LOW && vehicalCount > 0) {
    delay(2000); // delay for vehicle need to be in a position
    openBarrier();
    PlateEntry newExit;
    newExit.plateNumber = "NULL-Exit";
    newExit.time = currentTime; // Use the current timestamp
    plateHistory.push_back(newExit);
    delay(barrierDelay);
    vehicalCount--;
    closeBarrier();
    currentStatus = "Idle";
    server.handleClient(); // Update status on webpage
  }
}
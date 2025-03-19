#include <WiFi.h>
#include <esp_now.h>

// Pin Configurations
#define PIN_TRIG 23
#define PIN_ECHO 22

// Requirement Configuration
#define PARKING_MIN_DISTANCE_IN_CM 50
#define uS_TO_S 1000000
#define DUTY_CYCYLE_X 20 // Leader persons code => 10972565

#define OCCUPIED_MSG "OCCUPIED"
#define FREE_MSG "FREE"

// Change it to define to activate the proposed optimized solution-4
#undef OPTIMIZE_SENDING_FREQUENCY

#ifdef OPTIMIZE_SENDING_FREQUENCY
// 0 = uninit ; 1 = free ; 2 = occupied
RTC_NOINIT_ATTR uint8_t lastState = 0;
uint8_t currState = 0;
#endif /* OPTIMIZE_SENDING_FREQUENCY */

// ESP32 MAC receiver (set as a broadcast MAC adress)
uint8_t broadcastAddress[] = {0x8C, 0xAA, 0xB5, 0x84, 0xFB, 0x90};

// ESP32 NOW protocol handle
esp_now_peer_info_t peerInfo;

// Callback func after sending a data
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Ok" : "Error");

  esp_deep_sleep_start();
}

// Callback func after receiving a data
void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
  Serial.print("Message received: ");
  char receivedString[len];
  memcpy(receivedString, data, len);
  Serial.println(String(receivedString));
}


void setup () {
  String message = "";
  unsigned long distance_cm;
  // Initialize UART and HC-SR04 interface
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // Getting data from sensor
  // Start a new measurement:
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  // Read the result:

  distance_cm = (pulseIn(PIN_ECHO, HIGH) / 58);
  
  Serial.print("Distance in CM: ");
  Serial.println(distance_cm);
  Serial.println("");

  if (distance_cm < PARKING_MIN_DISTANCE_IN_CM) {
    message = OCCUPIED_MSG;
#ifdef OPTIMIZE_SENDING_FREQUENCY
    currState = 2;
#endif /* OPTIMIZE_SENDING_FREQUENCY */
  }
  else {
    message = FREE_MSG;
#ifdef OPTIMIZE_SENDING_FREQUENCY    
    currState = 1;
#endif /* OPTIMIZE_SENDING_FREQUENCY */
  }


#ifdef OPTIMIZE_SENDING_FREQUENCY
  if (currState != lastState) {
    Serial.println("State changed, will be sent to the sink node");
    lastState = currState;
  }
  else {
    Serial.println("State didn't change, going to deep sleep...");
    esp_sleep_enable_timer_wakeup(DUTY_CYCYLE_X * uS_TO_S);
    esp_deep_sleep_start();
  }
#endif /* OPTIMIZE_SENDING_FREQUENCY */
  
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_2dBm);
  esp_now_init();
  //send callback
  esp_now_register_send_cb(OnDataSent);
  //receive callback
  //esp_now_register_recv_cb(OnDataRecv);
  // Peer Registration
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // Add peer (Is possible to register multiple peers)
  if (ESP_OK != esp_now_add_peer(&peerInfo)) {
    //Serial.println("Failed to add peer");
    return;
  }

  esp_now_send(broadcastAddress, (uint8_t*)message.c_str(), message.length() + 1);
  esp_sleep_enable_timer_wakeup(DUTY_CYCYLE_X * uS_TO_S);

}

void loop () {
  // Dummy Part
  delay(10);
}

// 1) Deep Sleep
// 2) IDLE ; Setup to WiFi(excluding) 
// 3) Wifi ON ; Wifi to esp_send_now (excluding)
// 4) Transmission : esp_send_now to deep_sleep

#if 0
uint32_t startTime;
uint32_t endTime;
startTime = micros();
endTime = micros();
Serial.print("Initial Function takes in ms ");
Serial.print(endTime-startTime);
Serial.println("");
#endif // 0
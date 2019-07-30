#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include "ads1292.h"
#include "filter.h"

#define PACKET_TX_BUFFER_NUMBER 30    // 1 packet / 300 msec x 3 sec
#define PACKET_TX_BUFFER_SIZE   1205   // 2 Channel x 300 records x 2 byte + 5 header byte (ADS1292 1000 SPS)
#define PACKET_START_1          0xCA
#define PACKET_START_2          0xFE
#define PACKET_END              0xCD
#define PACKET_DEVICE_ID_OFFSET 2
#define PACKET_PACKET_ID_OFFSET 3
#define PACKET_FIRST_OFFSET     4
#define PACKET_LAST_OFFSET    (PACKET_TX_BUFFER_SIZE - 1)
#define PACKET_INDEX_OF_ID(x) (x % PACKET_TX_BUFFER_NUMBER)

//== ADS1292 variables ===============================
ads1292 ADS1292;

volatile int32_t s32DaqVals[2];
volatile byte SPI_RX_Buff[15] ;
volatile static int SPI_RX_Buff_Count = 0;
volatile char *SPI_RX_Buff_Ptr;
volatile bool ads1292dataReceived = false;
volatile byte LeadStatus=0;
volatile bool leadoff_detected = true;
int16_t raw_ecg[2], filtered_ecg[2];
volatile bool is_ads1292_init = false;

volatile uint8_t DRDY_counter = 0;
//== end ADS1292 variables ===========================

//== packet variables ================================
uint8_t packet_TX_Buffer[PACKET_TX_BUFFER_NUMBER][PACKET_TX_BUFFER_SIZE] = {0};
uint8_t packet_Id = 0;
uint8_t sending_packet_Id = 0;
uint16_t packet_Offset = PACKET_FIRST_OFFSET;
//== end packet variables ============================

//== WIFI variables ==================================
uint8_t deviceId = 0;
String networkName = "";
String networkPswd = "";
String udpAddress = "192.168.0.1";
uint16_t udpPort = 0;
boolean is_wifi_connected = false;
boolean is_wifi_connecting = false;
WiFiUDP udp;
Preferences preference;
//== END WIFI variables ===============================

//= = TASKS ============================================
void LEDTask(void *pvParameters);
void UDPTask(void *pvParameters);
void ATCommandTask(void * pvParameters);
void DRDYHandler(void);
TaskHandle_t loop_task = NULL;
//== END TASKS ========================================

void init_packet() {
  // Initital Empty packet
  for (byte i=0; i<PACKET_TX_BUFFER_NUMBER; i++) {
    memset((void*)&packet_TX_Buffer[i], 0, PACKET_TX_BUFFER_SIZE);
    packet_TX_Buffer[i][0] = PACKET_START_1;
    packet_TX_Buffer[i][1] = PACKET_START_2;
    packet_TX_Buffer[i][PACKET_DEVICE_ID_OFFSET] = deviceId;
    packet_TX_Buffer[i][PACKET_LAST_OFFSET] = PACKET_END;
  }
}

static inline void fill_packet(int16_t ch1, int16_t ch2) {
  packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][packet_Offset++] = (ch1 >> 8);
  packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][packet_Offset++] = (ch1 & 0xFF);
  packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][packet_Offset++] = (ch2 >> 8);
  packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][packet_Offset++] = (ch2 & 0xFF);
  if (packet_Offset >= PACKET_LAST_OFFSET) {
    packet_Id++;
    packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][PACKET_PACKET_ID_OFFSET] = packet_Id;
    packet_Offset = PACKET_FIRST_OFFSET;
  }
}

static inline void send_packet() {
  static uint8_t ret = 0;
  if (packet_Id != sending_packet_Id) {
    ret = udp.beginPacket(udpAddress.c_str(), udpPort);
    if (ret != 1) {
      log_e("Begin packet error");
    }
    udp.write(&packet_TX_Buffer[PACKET_INDEX_OF_ID(sending_packet_Id)][0], PACKET_TX_BUFFER_SIZE);
    ret = udp.endPacket();
    if (ret != 1) {
      log_e("End packet error");
    }
    sending_packet_Id++;
  }
}

// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      //When connected set 
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());  
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(udpPort);
      is_wifi_connecting = false;
      is_wifi_connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      is_wifi_connected = false;
      break;
    default:
      break;
  }
}

static inline void connectToWiFi() {
  if (is_wifi_connecting) return;
  is_wifi_connecting = true;
  Serial.println("Connecting to WiFi network: " + networkName);
  // delete old config
  WiFi.disconnect(true);
  //Initiate connection
  WiFi.begin(networkName.c_str(), networkPswd.c_str());
  Serial.println("Waiting for WIFI connection...");
}

static inline void init_preference() {
  preference.begin("ADS1292");
  deviceId = preference.getUChar("ID", 0);
  networkName = preference.getString("SSID", "0000");
  networkPswd = preference.getString("PWD", "");
  udpAddress = preference.getString("UDP", "");
  udpPort = preference.getUShort("PORT", 0);
  preference.end();
}
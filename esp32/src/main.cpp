#include "main.h"

void setup() {
  // setup pin
  pinMode(ADS1292_DRDY_PIN, INPUT);
  pinMode(ADS1292_START_PIN, OUTPUT);
  pinMode(ADS1292_CS_PIN, OUTPUT);
  Serial.begin(115200);
  // inititial packet
  init_packet();
  init_preference();
  // create RTOS task
  vTaskPrioritySet(NULL, 2); 
  xTaskCreate(LEDTask, "LED", 1000, NULL, 3, NULL);
  xTaskCreate(UDPTask, "UDP", 4096, NULL, 2, NULL);
  xTaskCreate(ATCommandTask, "ATCom", 4096, NULL, 1, NULL);
  // ADS init
  ADS1292.spi_Init(18, 19, 23, 4);
  ADS1292.ads1292_Init();  //initalize ADS1292 slave
}

uint8_t i,j;
uint32_t uecgtemp;
int32_t secgtemp, status_byte;
void loop() {

  if (digitalRead(ADS1292_DRDY_PIN) == LOW) {
    SPI_RX_Buff_Ptr = ADS1292.ads1292_Read_Data(); // Read the data,point the data to a pointer
    for (i = 0; i < 9; i++)
    {
      SPI_RX_Buff[SPI_RX_Buff_Count++] = *(SPI_RX_Buff_Ptr + i);  // store the result data in array
    }
    ads1292dataReceived = true;
  }

  if (ads1292dataReceived) {
    j = 0;
    for (i = 3; i < 9; i += 3)         // data outputs is (24 status bits + 24 bits Respiration data +  24 bits ECG data)
    {

      uecgtemp = (unsigned long) (  ((unsigned long)SPI_RX_Buff[i + 0] << 16) | ( (unsigned long) SPI_RX_Buff[i + 1] << 8) |  (unsigned long) SPI_RX_Buff[i + 2]);
      uecgtemp = (unsigned long) (uecgtemp << 8);
      secgtemp = (signed long) (uecgtemp);
      secgtemp = (signed long) (secgtemp >> 8);

      s32DaqVals[j++] = secgtemp;  //s32DaqVals[0] is Channel 1 and s32DaqVals[1] is Channel 2
    }

    status_byte = (long)((long)SPI_RX_Buff[2] | ((long) SPI_RX_Buff[1]) <<8 | ((long) SPI_RX_Buff[0])<<16); // First 3 bytes represents the status
    status_byte  = (status_byte & 0x0f8000) >> 15;  // bit15 gives the lead status
    LeadStatus = (unsigned char ) status_byte ;  

    if(!((LeadStatus & 0x1f) == 0 ))
      leadoff_detected  = true; 
    else
      leadoff_detected  = false; 
      //ecg_wave_buff[0] = (int16_t)(s32DaqVals[0] >> 8) ;  // ignore the lower 8 bits out of 24bits
      //ecg_wave_buff[1] = (int16_t)(s32DaqVals[1] >> 8) ;  
    if (leadoff_detected == false) {
      //ECG_ProcessCurrSample(&ecg_wave_buff[0], &ecg_filterout[0]);   // filter out the line noise @40Hz cutoff 161 order
      //ECG_ProcessCurrSample(&ecg_wave_buff[1], &ecg_filterout[1]);
      //QRS_Algorithm_Interface(ecg_filterout[0]);             // calculate 
    } else {
      //ecg_filterout[0] = 0;
      //ecg_filterout[1] = 0;
    }
  }
  ads1292dataReceived = false;
  SPI_RX_Buff_Count = 0;
}

void LEDTask(void *pvParameters) {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void UDPTask(void *pvParameters) {
  connectToWiFi();
  while(true) {
    //only send data when connected
    if(is_wifi_connected){
      Serial.println("WIFI Connected");
      //Send a packet
      //udp.beginPacket(udpAddress.c_str(), udpPort);
      //udp.printf("Seconds since boot: %u", (uint16_t)(millis()/1000));
      //udp.endPacket();
    }
    //Wait for 1 second
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void ATCommandTask(void *pvParameters)
{
  int state = -1, index_of_equal;
  bool isReboot = false;
  int8_t b;
  String tempStr;
  Serial.println("ATCommand Task");
  for(;;) {
    while ((b = Serial.read()) != -1) {
      if (b == 'A') state = 0;
      else if (state == 0 && b =='T') {
        Serial.println("OK");
        // Read String after AT until \n
        String command = Serial.readStringUntil('\n');
        // Find position of = in command string
        index_of_equal = command.indexOf('=');
        if (index_of_equal > 0) {
          // if '=' is found, parse and prepare the input value
          tempStr = command.substring(command.indexOf('=')+1);
          tempStr.replace('\r', '\0');
        }
        preference.begin("ADS1292");
        // ATID, ATID=xxxx 
        // Set device id to xxxx
        if (command.startsWith("ID")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("ID", tempStr);
            isReboot = true;
          } else {
            Serial.printf("%s\n", preference.getString("ID", "").c_str());
          }
        } 
        // ATSSID, ATSSID=xxxx
        // Set WIFI SSID that want to connect
        else if (command.startsWith("SSID")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("SSID", tempStr);
            isReboot = true;
          } else {
            Serial.printf("%s\n", preference.getString("SSID", "").c_str());
          }
        }
        // ATPWD, ATPWD=xxxx
        // Set WIFI Password
        else if (command.startsWith("PWD")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("PWD", tempStr);
            isReboot = true;
          } else {
            String pwd = preference.getString("PWD", "");
            for (int i=0; i<pwd.length(); i++) if (i>1 && i!=pwd.length()-1) Serial.print(pwd[i]);
          }
        }
        // ATUDP, ATUDP=0.0.0.0
        // Set IP address of target UDP server
        else if (command.startsWith("UDP")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("UDP", tempStr);
            isReboot = true;
          } else {
            Serial.printf("%s\n", preference.getString("UDP", "").c_str());
          }
        }
        // ATPORT, ATPORT=0000
        // Set port of target UDP server
        else if (command.startsWith("PORT")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("PORT", tempStr);
            isReboot = true;
          } else {
            Serial.printf("%s\n", preference.getString("PORT", "").c_str());
          }
        }
        preference.end();
        if (isReboot) ESP.restart();
      } else {
        state = -1;
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-stepper-motor-28byj-48-uln2003/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  Based on Stepper Motor Control - one revolution by Tom Igoe
*/

#include <Stepper.h>
#include <WiFi.h>

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution

// ULN2003 Motor Driver Pins
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17

// initialize the stepper library
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

// Wi-Fi 配置信息
const char* ssid = "srscore";
const char* password = "28727gh#";

WiFiUDP udp;
char Buffer[100];
int Len = 0;

IPAddress remoteIP(192, 168, 43, 204);

void setup() {
  // set the speed at 15 rpm
  myStepper.setSpeed(10);
  // initialize the serial port
  Serial.begin(115200);
    // 连接WiFi
  WiFi.begin(ssid, password);

  Serial.println("正在连接WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi已连接！");
  Serial.print("IP地址：");
  Serial.println(WiFi.localIP());
  udp.begin(23456);
}

void loop() {
    if (Serial.available()) {
    Buffer[Len] = Serial.read();
    if ((Buffer[Len] == '\r') || (Len >= 98)) {
      Buffer[Len + 1] = 0;
      Serial.println(Buffer);
      udp.beginPacket(remoteIP, 23456);
      udp.write((const uint8_t*)Buffer, Len);
      udp.endPacket();
      Len = 0;
    } else
      Len++;
  }
  int packetSize = udp.parsePacket();
  if (packetSize) {
    byte buffer[255];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len > 0) {
      // Serial.println(len);
      buffer[len] = 0;
      char charBuffer[len + 1];
      memcpy(charBuffer, reinterpret_cast<char*>(buffer), len + 1);
      int Permission;
      // 使用 sscanf 函数将两个数字提取并转换为整数
      if (sscanf(charBuffer, "%d", &Permission) == 1) {
        Serial.print("身份验证结果为");
        Serial.println(Permission);
        if(Permission){
          myStepper.step(2.2*stepsPerRevolution);
          delay(3000);
          myStepper.step(-1*stepsPerRevolution);
        }
      } else {
        Serial.println("无法解析");
      }
    int message = 1;
    udp.beginPacket(remoteIP, 23456);  // 修改为接收端的IP和端口
    udp.print(message);
    udp.endPacket();
    }
  }
}
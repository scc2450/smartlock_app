/*
////////////////
RC522
3.3V	3V3
RST	27
GND	GND
MISO	19
MOSI	23
SCK	18
SDA/SS	12
////////////////
舵机
橙色	13
红色	3V3
棕色	GND
////////////////
电机
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17 
////////////////
FPM383
V_Touch 3V3
Touch_OUT 14
VCC 3V3
TXD 32(黄线)
RXD 33(黑线)
GND GND
////////////////
*/
//Libraries
#include <SPI.h>//https://www.arduino.cc/en/reference/SPI
#include <MFRC522.h>//https://github.com/miguelbalboa/rfid
#include <ESP32Servo.h>
#include <string.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
//Constants
#define SS_PIN 12
#define RST_PIN 27
#define SERVO_PIN 13
//FPM383
#define Identify  //使用搜索指纹功能，如需使用注册指纹功能，请将这条语句注释
SoftwareSerial mySerial1(32,33);//创建软件串口通道（pin4与pin5）。接线注意【模块】的TX接4，RX接5
//Parameters
const int ipaddress[4] = {103, 97, 67, 25};
int decid=0;
//Variables
byte nuidPICC[4] = {0, 0, 0, 0};
MFRC522::MIFARE_Key key;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
IPAddress remoteIP(192, 168, 43, 89);
WiFiUDP udp;
// Wi-Fi 配置信息
const char* ssid = "srscore";
const char* password = "28727gh#";
// HTTP 请求目标URL
const char* serverUrl = "http://8.141.82.206:5000/receive_data"; // 请将 <your-computer-ip> 替换为您的计算机的 IP 地址

//指纹控制字
uint16_t ScanState = 0;
uint8_t PS_ReceiveBuffer[20];
uint8_t PS_SleepBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x33,0x00,0x37};
uint8_t PS_EmptyBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x0D,0x00,0x11};
uint8_t PS_GetImageBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x01,0x00,0x05};//指令01，录入图像指令，验证指纹时，探测手指，探测到后录入指纹图像存于图像缓冲区。返回确认码表示：录入成功、无手指等。
uint8_t PS_GetChar1Buffer[13] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x01,0x00,0x08};//指令02，生成特征于缓冲区1，返回确认码表示生成特征成功。
uint8_t PS_GetChar2Buffer[13] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x02,0x00,0x09};//指令02，生成特征于缓冲区2
uint8_t PS_BlueLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x03,0x01,0x01,0x00,0x00,0x49};//控制LED为蓝色
uint8_t PS_RedLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x02,0x04,0x04,0x02,0x00,0x50};//控制LED为红色
uint8_t PS_GreenLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x02,0x02,0x02,0x02,0x00,0x4C};//控制LED为绿色
uint8_t PS_WhiteLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x03,0x07,0x07,0x00,0x00,0x55};//控制LED为白色
uint8_t PS_SearchMBBuffer[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x04,0x01,0x00,0x00,0xFF,0xFF,0x02,0x0C};//指令04，搜索指纹库，以模板缓冲区中的特征文件搜索整个或部分指纹库。若搜索到，则返回页码。
uint8_t PS_AutoEnrollBuffer[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x31,'\0','\0',0x04,0x00,0x16,'\0','\0'}; 

void setup() {
  //Init Serial USB
  Serial.begin(115200);//这里在串口监视器（Serial Monitor）旁边数值要调成一样
  Serial.println("welcome!");
  Serial.println(F("Initialize System"));
  //init rfid D8,D5,D6,D7
  SPI.begin();
  rfid.PCD_Init();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();
  
  // 连接WiFi
  WiFi.begin(ssid, password);

  Serial.println("正在连接WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi已连接!");
  Serial.print("IP地址：");
  Serial.println(WiFi.localIP());
  udp.begin(23456);
  mySerial1.begin(57600); 
  Serial.println("UDP传输已开启\n软串口已开启(57600)");
  PS_Sleep();
  attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
  Serial.println("指纹触发器已开启");
}

void loop() {
  readRFID();
  if(ScanState == 0x10){
    PS_ControlLED(PS_BlueLEDBuffer);
    delay(50);
    #ifdef Identify
    PS_Identify();
    #else
    PS_Enroll(0);
    #endif
    delay(600);
    PS_Sleep();
    ScanState = 0;
    attachInterrupt(digitalPinToInterrupt(14),InterruptFun,RISING);
  }
  sendRFID();
  ReadSerial();
  ReceiveUdp();
}

void readRFID(void ) { /* function readRFID */
  ////Read RFID card
  unsigned char status;   //状态变量
  unsigned char str[5];
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  // Look for new 1 cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if (  !rfid.PICC_ReadCardSerial())
    return;

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  unsigned char* id = nuidPICC;
  decid=0;
  for(int i=0;i<4;i++) {  
    decid=decid+id[i]*pow(16,i);    //16转10进制
  }
  Serial.print(F("RFID In dec: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  Serial.print("DECID:");
  Serial.println(decid);   //输出卡DECID，10进制
  // 如果传感器/读卡器上没有新卡，则返回主程序。这将在空闲时保存整个进程。
  if ( ! rfid.PICC_IsNewCardPresent())
   return;
  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void sendRFID(){
  if(decid!=0){
    HTTPClient http;
    http.begin(serverUrl);
    String data = String("{\"decid\": ") + decid + "}";
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(data);
    if (httpResponseCode > 0) {
      Serial.printf("HTTP响应代码：%d\n", httpResponseCode);
      String response = http.getString();
      Serial.println("服务器响应：");
      Serial.println(response);
      String message;
      if(httpResponseCode==200){
        message = "1";
        mySerial1.write(PS_GreenLEDBuffer,16);
      }
      else if(httpResponseCode==201){
        message = "0";
        mySerial1.write(PS_RedLEDBuffer,16);
      }
      udp.beginPacket(remoteIP, 23456);  // 修改为接收端的IP和端口
      udp.print(message);
      udp.endPacket();
    } 
    else {
      Serial.printf("HTTP请求失败：%s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
    decid=0;
    delay(500);
  }
}
void ReadSerial(){
  char Buffer[100];
  int Len = 0;
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
}

void ReceiveUdp(){
  int packetSize = udp.parsePacket();
  if (packetSize) {
    byte buffer[255];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len > 0) {
      buffer[len] = 0;
      char charBuffer[len + 1];
      memcpy(charBuffer, reinterpret_cast<char*>(buffer), len + 1);
      String message;
      if (sscanf(charBuffer, "%c", &message) == 1) {
        Serial.print(message);
      } else {
        Serial.println("无法解析");
      }
    }
  }
}
//发送指令数据包，自动接受应答包
void FPM383C_SendData(int len,uint8_t PS_Databuffer[])
{
  mySerial1.write(PS_Databuffer,len);/*  */
  while(mySerial1.read() >= 0);
  memset(PS_ReceiveBuffer,0xFF,sizeof(PS_ReceiveBuffer));
}
//在一定时限内接受指令应答包
void FPM383C_ReceiveData(uint16_t Timeout)
{
  uint8_t i = 0;
  while(mySerial1.available() == 0 && (--Timeout))
  {
    delay(1);
  }
  while(mySerial1.available() > 0)
  {
    delay(2);
    PS_ReceiveBuffer[i++] = mySerial1.read();
    if(i > 15) break; 
  }
}

void PS_Sleep()
{
  FPM383C_SendData(12,PS_SleepBuffer);
}

void PS_ControlLED(uint8_t PS_ControlLEDBuffer[])
{
  FPM383C_SendData(16,PS_ControlLEDBuffer);
}

uint8_t PS_GetImage()
{
  FPM383C_SendData(12,PS_GetImageBuffer);
  FPM383C_ReceiveData(20000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;//应答包第7位为包标识，第9位为确认码
}

uint8_t PS_GetChar1()
{
  FPM383C_SendData(13,PS_GetChar1Buffer);
  FPM383C_ReceiveData(20000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}

uint8_t PS_GetChar2()
{
  FPM383C_SendData(13,PS_GetChar2Buffer);
  FPM383C_ReceiveData(20000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}

uint8_t PS_SearchMB()
{
  FPM383C_SendData(17,PS_SearchMBBuffer);
  FPM383C_ReceiveData(20000);
  // for(int i=0;i<22;i++){
  //   Serial.print(PS_ReceiveBuffer[i],HEX);
  //   Serial.print(" ");
  // }Serial.println();
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}

uint8_t PS_AutoEnroll(uint16_t PageID)
{
  PS_AutoEnrollBuffer[10] = (PageID>>8);
  PS_AutoEnrollBuffer[11] = (PageID);
  PS_AutoEnrollBuffer[15] = (0x54+PS_AutoEnrollBuffer[10]+PS_AutoEnrollBuffer[11])>>8;
  PS_AutoEnrollBuffer[16] = (0x54+PS_AutoEnrollBuffer[10]+PS_AutoEnrollBuffer[11]);
  FPM383C_SendData(17,PS_AutoEnrollBuffer);
  FPM383C_ReceiveData(10000);
  return PS_ReceiveBuffer[6] == 0x07 ? PS_ReceiveBuffer[9] : 0xFF;
}

uint8_t PS_Enroll(uint16_t PageID)
{
  if(PS_AutoEnroll(PageID) == 0x00)
  {
    PS_ControlLED(PS_GreenLEDBuffer);
    return PS_ReceiveBuffer[9];
  }
  PS_ControlLED(PS_RedLEDBuffer);
  return 0xFF;
}

void PS_Identify(){ 
  int finger = -1;
  int passed = 0;
  if(PS_GetImage() == 0x00){
    if(PS_GetChar1() == 0x00){
      if(PS_SearchMB() == 0x00){
        if(PS_ReceiveBuffer[9] == 0x00){
          passed = 1;
          finger = int(PS_ReceiveBuffer[11]);
          PS_ControlLED(PS_GreenLEDBuffer);
          Serial.print("用户");
          Serial.print(finger);
          Serial.println("指纹认证通过");
        }
      }
    }
  }
  if(!passed){
    PS_ControlLED(PS_RedLEDBuffer);
    Serial.println("指纹认证失败");
  }
  HTTPClient http;
  http.begin(serverUrl);
  String data = String("{\"fingerID\": ") + finger + "}";
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(data);
  if (httpResponseCode > 0) {
    Serial.printf("HTTP响应代码：%d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("服务器响应：");
    Serial.println(response);
    udp.beginPacket(remoteIP, 23456);  // 修改为接收端的IP和端口
    udp.print(int(finger>=0));
    udp.endPacket();
  }
  return;
}

void InterruptFun()
{
  Serial.println("检测到指纹触发器");
  detachInterrupt(digitalPinToInterrupt(14));
  ScanState |= 1<<4;
}
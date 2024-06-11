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
//#include <ESP32Servo.h>
#include <string.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
//Constants
#define SS_PIN 12
#define RST_PIN 27
// #define SERVO_PIN 13
//FPM383
#define Identify  //使用搜索指纹功能，如需使用注册指纹功能，请将这条语句注释
SoftwareSerial mySerial1(32,33);//创建软件串口通道（pin4与pin5）。接线注意【模块】的TX接4，RX接5。用于指纹
SoftwareSerial FM225_Serial(17,16);//创建软件串口通道（pin16与pin17）。接线注意【模块】的TX接16，RX接17。用于人脸
//Parameters
const int ipaddress[4] = {103, 97, 67, 25};
int decid=0;
char Buffer[100];
int Len = 0;

//Parameter for FM225
int FM225_state = 0;
int NumofID = 0;

//Variables
byte nuidPICC[4] = {0, 0, 0, 0};
MFRC522::MIFARE_Key key;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
IPAddress remoteIP(192, 168, 43, 170);
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

//FM225控制字
uint8_t FM225_ReceiveBuffer[210];
uint8_t set_standby[6] = { 0xEF, 0xAA, 0x23, 0x00, 0x00, 0x23 };                   //待机
uint8_t get_status[6] = { 0xEF, 0xAA, 0x11, 0x00, 0x00, 0x11 };                    //获取模组当前状态
uint8_t go_recognization[8] = { 0xEF, 0xAA, 0x12, 0x00, 0x02, 0x00, 0x0A, 0x1A };  // 匹配超时时间10s, 校验位0x1A
uint8_t get_usernameANDid[6] = { 0xEF, 0xAA, 0x24, 0x00, 0x00, 0x24 };             //获取已注册用户数量及ID
uint8_t del_all[6] = { 0xEF, 0xAA, 0x21, 0x00, 0x00, 0x21 };
uint8_t register_face[46] = { 0xEF, 0xAA, 0x26, 0x00, 0x28, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00 };
//从第七位起，其后32位为用户名。最后一位为校验位。这些需要在注册时随时更新。
uint8_t get_number[6] = { 0xEF, 0xAA, 0x24, 0x00, 0x00, 0x24 };
//按键
const int buttonPin1 = 25;  //25口，用于人脸识别
const int buttonPin2 = 26;  //26口，用于人脸注册
const int buttonPin3 = 35;  //35口，用于删除


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
/*void ReadSerial(){
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
}*/

void ReceiveUdp(){
  int packetSize = udp.parsePacket();
  if (packetSize) {
    byte buffer[255];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len > 0) {
      buffer[len] = 0;
      char charBuffer[len + 1];
      memcpy(charBuffer, reinterpret_cast<char*>(buffer), len + 1);
      int message;
      if (sscanf(charBuffer, "%d", &message) == 1) {
        Serial.println(message);
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
    udp.print(int(finger>0));
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

//FM225
void FM225_SendData(int len, uint8_t FM225_Databuffer[]) {
  Serial.print("Send data:");
  for (int i = 0; i < len; i++) {
    FM225_Serial.write(FM225_Databuffer[i]);
    Serial.print(FM225_Databuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println(";");
  //while(FM225_Serial.read() >= 0);
  memset(FM225_ReceiveBuffer, 0xFF, sizeof(FM225_ReceiveBuffer));
}

//在一定时限内接受指令应答包
void FM225_ReceiveData(uint16_t Timeout) {
  uint8_t i = 0;
  Serial.print("Received data:");
  while (FM225_Serial.available() == 0 && (--Timeout)) {
    delay(1);
  }
  if (FM225_Serial.available() > 0) {
    memset(FM225_ReceiveBuffer, 0, sizeof(FM225_ReceiveBuffer));
    while (FM225_Serial.available() > 0) {
      uint8_t ThisData = FM225_Serial.read();
      delay(2);
      if (ThisData == 0xEF) {
        i = 0;
      }
      FM225_ReceiveBuffer[i] = ThisData;
      Serial.print(FM225_ReceiveBuffer[i++], HEX);
      //Serial.print("(left:");
      //Serial.print(FM225_Serial.available());
      Serial.print(" ");
      if (i > 210) break;
    }
    Serial.println(";");
  }
}

void FM225_Sleep() {
  FM225_SendData(6, set_standby);
  FM225_ReceiveData(200);
}

void FM225_GetStatus() {
  FM225_SendData(6, get_status);
}

uint8_t GetParityCheck(uint8_t *data, int length)  //校验：出去EF、AA后，按位异或
{
  uint8_t nParityCheck = 0;
  for (int i = 2; i < length - 1; ++i) {
    nParityCheck ^= data[i];
  }
  return nParityCheck;
}

//检测到相应信号，进入工作状态

int FM225_Button(int buttonPin) {
  int reading = digitalRead(buttonPin);  //将开关状态保存在reading内
  int i11 = 0;
  while (reading) {
    i11++;
    if (i11 == 50) return 1;
  }
  return 0;
}

int LastPassTime=0;//记录识别通过后的时间
void FM225_Identify() {
  uint8_t IfSuccess[2];
  FM225_SendData(8, go_recognization);
   int message = 0;         
  while (FM225_state == 1) {
    FM225_ReceiveData(2000);
    IfSuccess[0] = FM225_ReceiveBuffer[7];
    IfSuccess[1] = FM225_ReceiveBuffer[8];       //两位ID位                           
    if (IfSuccess != 0 && FM225_ReceiveBuffer[5] == 0x12 && FM225_ReceiveBuffer[6] == 0x00)  //有ID，响应的命令为0x12，响应成功;
    {
      Serial.println("人脸识别通过");
      FM225_state = 0;  //sleep
      FM225_Sleep();
      message=1;
      LastPassTime=millis();
      //Serial.print("通过时间:");
      //Serial.println(LastPassTime);
      //PS_ControlLED(PS_GreenLEDBuffer);
      break;
    } 
    else if (FM225_ReceiveBuffer[2] == 0x01) {
      Serial.println("正在匹配");
      message=0;
    }
    else{
      Serial.println("人脸识别失败");
      FM225_state = 0;
      FM225_Sleep();
      message=0;
      break;
    } 
  }
  HTTPClient http;
  http.begin(serverUrl);
  int face = -1;
  if(int(IfSuccess[0])>=0){
    face = int(IfSuccess[0]);
  };
  String data = String("{\"faceID\": ") + face + "}";
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(data);
  if (httpResponseCode > 0) {
    Serial.printf("HTTP响应代码：%d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("服务器响应：");
    Serial.println(response);
    udp.beginPacket(remoteIP, 23456);  // 修改为接收端的IP和端口
    udp.print(int(face>0));
    udp.endPacket();
  }
}

//删除所有用户
void DelAllUsers() {
  FM225_SendData(6, del_all);
  FM225_ReceiveData(200);
  Serial.println("已删除所有用户");
}

//人脸注册
void RegisterFace() {
  int DeltaTime=0;
  DeltaTime=millis()-LastPassTime;
  Serial.print("距上次识别成功");
  Serial.print(DeltaTime);
  Serial.println("毫秒");
  if(DeltaTime<=5000){
  register_face[7] = ++NumofID;
  Serial.print("正在注册第");
  Serial.print(NumofID);
  Serial.println("位用户");
  register_face[45] = GetParityCheck(register_face, 46);
  FM225_SendData(46, register_face);
  uint8_t IfSuccess[2];
  int i=0;
  while(1){
  FM225_ReceiveData(2000);
  IfSuccess[0] = FM225_ReceiveBuffer[7];
  IfSuccess[1] = FM225_ReceiveBuffer[8];
  if (IfSuccess != 0 && FM225_ReceiveBuffer[6] == 0x00 && i<10 && FM225_ReceiveBuffer[0] == 0xEF&&FM225_ReceiveBuffer[2] == 0x00) {
    Serial.println("注册成功!");
    PS_ControlLED(PS_GreenLEDBuffer);
    break;
  } 
  else if(FM225_ReceiveBuffer[2]==0x01 && i<10){
    Serial.println("正在注册");
    i++;
  }
  else 
  {
    Serial.println("注册失败");
    NumofID--;
    PS_ControlLED(PS_RedLEDBuffer);
    break;
  }
  }
  }
  else{
    Serial.println("无注册权限!");
    PS_ControlLED(PS_RedLEDBuffer);
  }
}

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

  //FM225
  FM225_Serial.begin(115200);
  FM225_Sleep();
  pinMode(buttonPin1, INPUT);
  // pinMode(buttonPin2, INPUT);
  //pinMode(buttonPin3, INPUT);
  FM225_SendData(6, get_number);  //获取用户数量
  FM225_ReceiveData(20);
  NumofID = FM225_ReceiveBuffer[7];
  Serial.print("现有");
  Serial.print(NumofID);
  Serial.println("位用户");
  Serial.println("Setup complete.");
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
  //ReadSerial()
   if (Serial.available()) {
    Buffer[Len] = Serial.read();
    if ((Buffer[Len] == '\r') || (Len >= 98)) {
      Buffer[Len + 1] = 0;
      //Serial.println(Buffer);
      udp.beginPacket(remoteIP, 23456);
      udp.write((const uint8_t*)Buffer, Len);
      udp.endPacket();
      Len = 0;
    } else
      Len++;
  }
  ReceiveUdp();

  //FM225
  int state1, state2, state3 = 0;
  state2 = FM225_Button(buttonPin2);
  state1 = FM225_Button(buttonPin1);
  //取消删除功能
  //state3 = FM225_Button(buttonPin3);
  FM225_state = state1 + 2 * state2 + 5 * state3;
  if (FM225_state != 0) {
    Serial.print("button Number is:");
    Serial.println(FM225_state);
  }
  if (FM225_state == 2) {
    RegisterFace();
    FM225_state = 0;
  }
  /* else if (FM225_state == 5) {
    DelAllUsers();
    FM225_state = 0;
    NumofID = 0;
    delay(200);
  }*/
  while (FM225_state == 1) {
    FM225_Identify();
  }
}

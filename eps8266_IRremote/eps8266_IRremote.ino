#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>

const uint16_t kRecvPin = 14; //pin 14 (D5 on a NodeMCU)

IRrecv irrecv(kRecvPin);

decode_results results;

String key = "";

uint8 keyMode = 0; //当前按键的模式,类型如下
uint8 KEY_NULL = 0; //0空
uint8 CLICK_PRESS = 1; //1单点
uint8 HOLD_PRESS = 2; //2长按

unsigned long keyPressTime;
unsigned long preReceivePressTime; //上次接受到消息的时间
unsigned long longPressThreashold = 500;//收到hold请求,从按下到切换成长按的时间要大于400毫秒
unsigned long triggerTimeout = 305; //超时时间一定要大于loop delay时间. 毫秒之后触发trigger
String keyHoldSignal = "FFFFFFFFFFFFFFFF"; //hold信号

void keyRelease(){
  if(keyMode == CLICK_PRESS){
    Serial.printf("点按 key: %s \n", key.c_str());
    //todo 
  }else if(keyMode == HOLD_PRESS){
    Serial.printf("释放长按 key: %s \n", key.c_str());
    //todo 
  }else{
    Serial.printf("异常 keyMode: %d, key: %s ;\n", keyMode, key.c_str());
  }
  keyMode = KEY_NULL;
  key = "";
}

void longPressStart(){
  Serial.printf("长按 key: %s\n", key.c_str());
  //todo 
}

void IRReceive(){
  if(keyMode != KEY_NULL && key != ""){
    unsigned long timeout = triggerTimeout;
    //让长按的超时时间是普通点击的2倍,避免长按中途短时间信号丢失被释放.
    if(keyMode == HOLD_PRESS){
      timeout = 2*triggerTimeout;
    }
    if(millis() - preReceivePressTime > timeout ){
      keyRelease();
    }
  }

  if (!irrecv.decode(&results)) {
    return;
  }

  char keyBuffer[19];
  sprintf(keyBuffer, "%llX", results.value);
  String keyStr(keyBuffer);

  if(keyStr == keyHoldSignal && keyMode == CLICK_PRESS){
    //当按钮接收到hold请求超过500毫秒后,将点按切换成长按状态
    if(millis() - keyPressTime > longPressThreashold ){ 
      Serial.println(">>>change long hold>>");
      keyMode = HOLD_PRESS;
      longPressStart();
    }
  }else if(keyStr != keyHoldSignal && keyMode == KEY_NULL && keyStr.length() > 1 ){
    Serial.printf("pressssssssssss: %s \n", keyStr.c_str());
    key = keyStr;
    keyPressTime = millis();
    keyMode = CLICK_PRESS;
  }
  if(keyStr == keyHoldSignal){
    Serial.print(".");
  }else{
    Serial.println("----------------------");
  }
  irrecv.resume();  // Receive the next value
  preReceivePressTime = millis();
}

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();  // Start the receiver
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);
}

void loop() {
  IRReceive();
  delay(100);
}
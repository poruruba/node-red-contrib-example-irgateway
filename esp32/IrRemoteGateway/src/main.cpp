#include <Arduino.h>
#include <M5Atom.h>

#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

#include <WiFi.h>
#include <WiFiUdp.h>

#include <ArduinoJson.h>

#include <Adafruit_NeoPixel.h>

#define WIFI_SSID "【WiFiアクセスポイントのSSID 】"
#define WIFI_PASSWORD "【WiFIアクセスポイントのパスワード】"

#define UDP_HOST  "【Node-RED稼働ホスト名またはIPアドレス】"
#define UDP_SEND_PORT 1401
#define UDP_RECV_PORT 1402

#define IR_SEND_PORT 33
#define IR_RECV_PORT 32

#define NUM_OF_PIXELS 25
#define PIXELS_PORT 27

#define JSON_CAPACITY 512

static StaticJsonDocument<JSON_CAPACITY> jsonDoc;
static IRsend irsend(IR_SEND_PORT);
static IRrecv irrecv(IR_RECV_PORT);
static decode_results results;
static WiFiUDP udp;
static Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_OF_PIXELS, PIXELS_PORT, NEO_GRB + NEO_KHZ800);

static long wifi_connect(const char *ssid, const char *password);
static long process_ir_receive(void);
static long process_udp_receive(int packetSize);
static long process_button(uint8_t index);
uint8_t *parsehex(const char *p_message, uint32_t *p_len);

long udp_send(JsonDocument& json)
{
  int size = measureJson(json);
  char *p_buffer = (char*)malloc(size + 1);
  int len = serializeJson(json, p_buffer, size);
  p_buffer[len] = '\0';

  udp.beginPacket(UDP_HOST, UDP_SEND_PORT);
  udp.write((uint8_t*)p_buffer, len);
  udp.endPacket();

  free(p_buffer);

  return 0;
}

void setup() {
  // put your setup code here, to run once:
  M5.begin(true, true, false);

  irsend.begin();
  irrecv.enableIRIn();

  pixels.begin();
  pixels.clear();
  pixels.show();

  wifi_connect(WIFI_SSID, WIFI_PASSWORD);

  udp.begin(UDP_RECV_PORT);
}

void loop() {
  // put your main code here, to run repeatedly:

  M5.update();

  if (irrecv.decode(&results)) {
    process_ir_receive();
    irrecv.resume(); 
  }

  int packetSize = udp.parsePacket();
  if( packetSize > 0 ){
    process_udp_receive(packetSize);
  }

  if( M5.Btn.wasPressed() ){
    process_button(0);
  }
}

static long process_ir_receive(void)
{
  Serial.println("process_ir_receive");

  if(results.overflow){
    Serial.println("Overflow");
    return -1;
  }
  if( results.decode_type != decode_type_t::NEC || results.repeat ){
    Serial.println("not supported");
    return -1;
  }

  Serial.print(resultToHumanReadableBasic(&results));
  Serial.printf("address=%d, command=%d\n", results.address, results.command);

  jsonDoc.clear();
  jsonDoc["type"] = "ir_received";
  jsonDoc["address"] = results.address;
  jsonDoc["command"] = results.command;
  jsonDoc["value"] = results.value;

  udp_send(jsonDoc);

  return 0;
}

static long process_udp_receive(int packetSize)
{
  Serial.println("process_udp_receive");

  char *p_buffer = (char*)malloc(packetSize + 1);
  if( p_buffer == NULL )
    return -1;
  
  int len = udp.read(p_buffer, packetSize);
  if( len <= 0 ){
    free(p_buffer);
    return -1;
  }
  p_buffer[len] = '\0';

  DeserializationError err = deserializeJson(jsonDoc, p_buffer);
  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.f_str());

    free(p_buffer);
    return -1;
  }

  const char *p_type = jsonDoc["type"];
  Serial.printf("type=%s\n", p_type);
  if( strcmp(p_type, "ir_send") == 0 ){
    if( jsonDoc.containsKey("value") ){
      uint32_t value = jsonDoc["value"];
      irsend.sendNEC(value);
    }else{
      uint16_t address = jsonDoc["address"];
      uint16_t command = jsonDoc["command"];
      uint32_t value = irsend.encodeNEC(address, command);
      irsend.sendNEC(value);
    }
  }else
  if( strcmp(p_type, "pixels_draw") == 0 ){
    const char *p_hex = jsonDoc["bitmap"];
    uint32_t bitmap_len;
    uint8_t *p_bitmap = parsehex(p_hex, &bitmap_len);
    if( p_bitmap == NULL ){
      Serial.println("parsehex error");
      free(p_buffer);
      return -1;
    }
    if( bitmap_len < ((NUM_OF_PIXELS + 7) / 8)){
      Serial.println("bitmap error");
      free(p_buffer);
      return -1;
    }

    uint32_t fore_color = jsonDoc["fore_color"];
    uint32_t back_color = jsonDoc["back_color"] | 0x000000;

    for( int i = 0 ; i < NUM_OF_PIXELS ; i++ ){
      if( (p_bitmap[i / 8] >> (7 - (i % 8))) & 0x01 )
        pixels.setPixelColor(i, fore_color);
      else
        pixels.setPixelColor(i, back_color);
    }
    pixels.show();

    free(p_bitmap);
  }else
  if( strcmp(p_type, "pixels_clear") == 0 ){
    pixels.clear();
    pixels.show();
  }else{
    Serial.println("Not supported");
    free(p_buffer);
    return -1;
  }

  free(p_buffer);

  return 0;
}

static long process_button(uint8_t index)
{
  Serial.println("process_button");

  jsonDoc.clear();
  jsonDoc["type"] = "button_pressed";
  jsonDoc["index"] = index;
  
  udp_send(jsonDoc);

  return 0;
}

static long wifi_connect(const char *ssid, const char *password)
{
  Serial.println("");
  Serial.print("WiFi Connenting");

  if( ssid == NULL && password == NULL )
    WiFi.begin();
  else
    WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nConnected : IP=");
  Serial.print(WiFi.localIP());
  Serial.print(" Mac=");
  Serial.println(WiFi.macAddress());

  return 0;
}

static uint8_t toB(char c)
{
  if( c >= '0' && c <= '9' )
    return c - '0';
  if( c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if( c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;
}

uint8_t *parsehex(const char *p_message, uint32_t *p_len)
{
  int len = strlen(p_message);
  if( len % 2 )
    return NULL;
  uint8_t *p_buffer = (uint8_t*)malloc(len / 2);
  if( p_buffer == NULL )
    return NULL;

  for( int i = 0 ; i < len ; i += 2 ){
    p_buffer[i / 2] = toB(p_message[i]) << 4; 
    p_buffer[i / 2] |= toB(p_message[i + 1]); 
  }

  *p_len = len / 2;

  return p_buffer;
}
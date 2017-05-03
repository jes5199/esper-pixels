#include <ESP8266WiFi.h>
#include <NeoPixelBus.h>

#ifdef ESP8266
extern "C" {
  #include "user_interface.h"
}
#endif

#define PixelCount 512

#define OpcServerPort  7890
#define MaximumConnectionCount 1

const char* ssid = "Catastrophe";
const char* password = "alloneword";

// fails silently if less than 8 character password
const char* esper_password = "EUNOIANS";

// hardware limitations put this on GPIO3, which is labeled "RX" on my board. Possibly also "D10".
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> pixels(PixelCount, 0);

// different hardware limitations but this on GPIO2, which is labeled "D4" on my board
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> pixels2(PixelCount, 0);

struct openPixelMachine {
  int messageState;
  int dataIndex;
  int messageLen;
  int channel;
  int command;
  int lastMillis;
};

WiFiServer server(OpcServerPort);
WiFiClient clients[MaximumConnectionCount];
openPixelMachine state[MaximumConnectionCount];

void configureAP() {
  struct softap_config *config = (struct softap_config *)malloc(sizeof(struct
softap_config));
  wifi_softap_get_config(config);
  
  Serial.printf("%s",config->ssid);
  Serial.println();
  
  sprintf((char*)config->ssid, "Esper✨%x", system_get_chip_id());
  sprintf((char*)config->password, esper_password);

  config->authmode = AUTH_WPA_WPA2_PSK;
  config->ssid_len = 0; // or its actual SSID length
  config->max_connection = 4;
  if(!wifi_softap_set_config(config)) {
    Serial.printf("Got False from setting config");
    Serial.println();
  }
  free(config);
}


void setup() {  
  //system_update_cpu_freq(SYS_CPU_160MHZ);

  Serial.print("\nConnecting to "); Serial.println(ssid);
  configureAP();
  WiFi.begin(ssid, password);
  
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("Could not connect to"); Serial1.println(ssid);
    while(1) delay(500);
  }
  //start UART and the server
  Serial.begin(115200);
  server.begin();
  server.setNoDelay(true);
  
  Serial.print("Ready! ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(OpcServerPort);
  
  //pixels.begin();
  //pixels.setBrightness(14);

  pixels.Begin();
  pixels2.Begin();

  for(int i=0;i<PixelCount;i++){
    int r = 50;
    int g = 50;
    int b = 50;
    int n = i % 6;
    // r - - - r r
    // - g - g - g
    // - - b b b -
    if(n > 0 && n < 4) { r = 0;}
    if(!(n % 2)) { g = 0;}
    if(n < 2 || n > 4) { b = 0;}

    pixels.SetPixelColor(i, RgbColor(r,g,b));
    pixels.Show();
  }

  for(int i=0;i<PixelCount;i++){
    int r = 50;
    int g = 50;
    int b = 50;
    int n = (i / 2) % 6;
    // r - - - r r
    // - g - g - g
    // - - b b b -
    if(n > 0 && n < 4) { r = 0;}
    if(!(n % 2)) { g = 0;}
    if(n < 2 || n > 4) { b = 0;}

    pixels2.SetPixelColor(i, RgbColor(r,g,b));
    pixels2.Show();
  }
  

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);

  pixels.Show();
  
  Serial.print("Let's Begin.");
  Serial.println();
}

template<typename T_COLOR_FEATURE, typename T_METHOD> void setPixel(NeoPixelBus<T_COLOR_FEATURE,T_METHOD> * pix, int n, int c, char byte) { 
  if(n < PixelCount) {
    RgbColor color = pix->GetPixelColor(n);
    if(c == 0) {
      color.R = byte;
    } else if(c == 1) {
      color.G = byte;
    } else {
      color.B = byte;
    }
    pix->SetPixelColor(n, color);
  }
}

void setPixelCh(int channel, int n, int c, char byte) {
  if(channel == 0 || channel == 1) {
    setPixel(&pixels, n, c, byte);
  }
  if(channel == 0 || channel == 2) {
    setPixel(&pixels2, n, c, byte);
  }
}

void handleByte(openPixelMachine* d, char byte) {
    if(d->messageState == 0) {
      d->messageState = 1;
      d->channel = byte;
    } else if(d->messageState == 1) {
      d->messageState = 2;
      d->command = byte;
    } else if(d->messageState == 2) {
      d->messageState = 3;
      d->messageLen = byte * 256;
    } else if(d->messageState == 3) {
      d->messageState = 4;
      d->messageLen += byte;
      d->dataIndex = 0;
    } else if(d->messageState == 4) {
      d->messageLen -= 1;
      if(d->command == 0) { // COLOR SOME PIXELS
        int pixelN = d->dataIndex / 3;
        int pixelC = d->dataIndex % 3;
        setPixelCh(d->channel, pixelN, pixelC, byte);
      }
      d->dataIndex += 1;
    }

    if(d->messageState == 4 && d->messageLen <= 0) {
      d->messageState = 0;
    }
}

void checkForClient() {
  if(!server.hasClient()) return;
  
  for(int i = 0; i < MaximumConnectionCount; i++){
    if (!clients[i] || !clients[i].connected()){
      if(clients[i]) clients[i].stop();
      clients[i] = server.available();
      Serial.println("OPC client connected!");
      memset((void*) &state[i], 0, sizeof(struct openPixelMachine));
      return;
    }
  }

  WiFiClient client = server.available();
  client.stop();  
}

void listenToClients() {
  for(int i = 0; i < MaximumConnectionCount; i++){
    if (clients[i] && clients[i].connected()){
      while(clients[i].available())
        handleByte(&state[i], clients[i].read());
    }
  }
}

void handleNetwork() {
  checkForClient();
  listenToClients();
}

int lastPixelMillis = 0;

int frames = 0;
int seconds = 0;


void loop() {
  handleNetwork();
  
  if(millis() - lastPixelMillis > 1000 / 1000) {
    digitalWrite(LED_BUILTIN, LOW);

    pixels.Show();
    pixels2.Show();
    lastPixelMillis = millis();
    frames += 1;
  }
  int now = millis() / 1000;
  if(now - seconds > 5) {
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.print(frames / 5);
    Serial.println(" fps");
    seconds = now;
    frames = 0;
  }
}

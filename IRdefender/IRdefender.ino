/*
    This sketch sets up a web server which allows a web client to change
    which team is harmed and how often the defender fires.

    It is based on code in the Arduino library (specifically code
    related to the ESP8266):
      Arduino/libraries/ESP8266WiFi/examples/WiFiManualWebServer/WiFiManualWebServer.ino
    and as documented at:
      https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/server-examples.html
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// #define DEBUG 1

// Fill these values in as appropriate for your WiFi.
const char* kSSID = "YOUR_SSID";
const char* kPASS = "YOUR_SSID_PASSWORD";

// port for the web server.
const uint16_t kPORT = 80;

// ESP8266 GPIO pin to use. Recommended: 4 (D2).
const uint16_t kIRLED= 4;
// Carrier frequency in Khz. Normally 38Khz for IR.
const int kCarrierFreqK = 38;

// Pin that has an LED which we can flash.
const int  kLED_PIN = 5;

// Default fire rate in milliseconds
const uint32_t kFIRE_RATE_MS = 1000;

typedef enum {
    FM_HARM_NONE   = 0,
    FM_HARM_RED    = 1,
    FM_HARM_BLUE   = 2,
    FM_HARM_BOTH   = 3
} fire_mode_t;

// Force a read or write every time.
// Because this is small (native size int?), assume for now that there is no
//  need for locking as long as the reads and writes are atomic.
//
// TODO(2018-12-26): Validate the size of enum and the max size
// of a single cycle read or write.
volatile fire_mode_t fm = FM_HARM_BOTH;

// Rogue firing pattern, harm everyone
uint16_t harmBothRawData[17] = {3164*2, 256*2, 264*2, 255*2, 796*2, 255*2, 275*2, 244*2, 796*2, 255*2, 291*2, 241*2, 263*2, 256*2, 794*2, 257*2, 794*2 };

// Red team firing pattern, harm blue
uint16_t harmBlueRawData[17] = {3167*2, 253*2, 277*2, 241*2, 793*2, 258*2, 278*2, 254*2, 797*2, 241*2, 279*2, 253*2, 262*2, 257*2, 799*2, 252*2, 262*2};

// Blue team firing patter, harm res
uint16_t harmRedRawData[17] = {3133*2, 269*2, 262*2, 257*2, 809*2, 254*2, 266*2, 239*2, 796*2, 254*2, 260*2, 271*2, 265*2, 253*2, 261*2, 270*2, 796*2};

IRsend irsend(kIRLED);  // Set the GPIO to be used to sending the message.

// Create something that can run in the background.
Ticker worker;
uint32_t rate_ms = kFIRE_RATE_MS;

// Can't send the pattern during the main loop() since that is
// only called after a network connection is made. We want this
// running in the background. For safety, don't do Serial.print().
//
// TODO(2018-12-26): Can we find a way to run this when waiting for a
// connection?  Arguably don't need it to run while someone is accessing
// the web page.
void run_me() {
  // Don't do any serial output here.
  int val = digitalRead(kLED_PIN);
  digitalWrite(kLED_PIN, !val);
  
  // Get a local copy so nothing changes out from under us.
  fire_mode_t fm_copy = fm;

  switch (fm_copy) {
    case FM_HARM_BLUE:
      irsend.sendRaw(harmBlueRawData, sizeof(harmBlueRawData) / sizeof(uint16_t), kCarrierFreqK);
      break;
    case FM_HARM_RED:
      irsend.sendRaw(harmRedRawData, sizeof(harmRedRawData) / sizeof(uint16_t), kCarrierFreqK);
      break;
    case FM_HARM_BOTH:
      irsend.sendRaw(harmBothRawData, sizeof(harmBothRawData) / sizeof(uint16_t), kCarrierFreqK);
      break;
    default:
      // either none or unknown. In either case, do nothing...
      break;
  }
}

// Create an instance of the server
WiFiServer server(kPORT);

void setup() {
  // initialize serial: We send stuff to the Serial()
  // even without DEBUG, so initialize always.
  Serial.begin(115200);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);

  Serial.println(" ");
  Serial.println("IR TCP Server");

  // Prepare visible LED
  pinMode(kLED_PIN, OUTPUT);
  digitalWrite(kLED_PIN, 0);

#ifdef DEBUG
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(kSSID);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(kSSID, kPASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");

  // Start the server
  server.begin();

  Serial.print("Server started on port ");
  Serial.println(kPORT);

  // Print the IP address
  Serial.println(WiFi.localIP());

  // Start up the ir sender
  irsend.begin();

#ifdef DEBUG
  // Start up the ticker and run it every N milliseconds.
  Serial.print("Starting ticker every ");
  Serial.print(kFIRE_RATE_MS);
  Serial.println("ms");
#endif
  worker.attach_ms(rate_ms, run_me);
}

// Loop is called from the outside world over and over again.
// We don't actually loop here, the outside world is doing the
// loop and we are called as part of that loop. With the webserver
// code, we are called once per web server connection. Thus, can't
// use this loop for timing, periodic tasks, background tasks, etc.
void loop() {
  // Check if a client has connected. Destructor called when we
  // exit the loop().
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.print("new client: ");
  Serial.println(client.remoteIP().toString());

  client.setTimeout(5000); // default is 1000

  // Read the first line of the request. Yes, this
  // can have buffer overrun issues, but GoodEnough (tm)
  // for now.
  String req = client.readStringUntil('\r');

#ifdef DEBUG
  Serial.println("request: ");
  Serial.println(req);
#endif

  bool change_rate = false;
  // We are not caring whether the request was a GET/POST or whatever. Since
  // we are changing state, we should make sure this is a POST, but it's
  // GoodEnough (tm) for now.

  // Match the request
  if (req.indexOf("/fm/0") != -1) {
    fm = FM_HARM_NONE;
  } else if (req.indexOf("/fm/1") != -1) {
    fm = FM_HARM_RED;
  } else if (req.indexOf("/fm/2") != -1) {
    fm = FM_HARM_BLUE;
  } else if (req.indexOf("/fm/3") != -1) {
    fm = FM_HARM_BOTH;
  } else if (req.indexOf("/faster") != -1) {
    if (rate_ms > 200) {
      rate_ms /= 2;
      change_rate = true;
    }
  } else if (req.indexOf("/slower") != -1) {
    if (rate_ms < 10000) {
      rate_ms *= 2;
      change_rate = true;
    }
  } else {
    Serial.println("invalid request");
  }

  // read/ignore the rest of the request
  // client.flush() is for output only, so don't use that here.
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  // Determine the "per firing mode" information here
  // so it can be inserted into the html output later.
  char *color;
  char *harm;
  switch (fm) {
  case FM_HARM_NONE:
    harm = "harm no one";
    color = "";
    break;
  case FM_HARM_RED:
    harm = "harm red";
    color = "<body style=\"background-color:indianred;\">";
    break;
  case FM_HARM_BLUE:
    harm = "harm blue";
    color = "<body style=\"background-color:lightblue;\">";
    break;
  case FM_HARM_BOTH:
    harm = "harm both red and blue";
    color = "<body style=\"background-color:violet;\">";
    break;
  default:
    harm = "unexpected";
    color = "";
  }
  
  // Send the response to the client
  // It is OK for multiple small client.print/write,
  // because nagle algorithm will group them into one single packet
  client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n");
  client.print(color);
  client.print("Fire mode is ");
  client.print(harm);
  client.print(".<br>Fire rate is ");
  client.print(rate_ms);
  client.print("ms.<br><br>Click <a href='http://");
  client.print(WiFi.localIP());
  client.print("/fm/0'>here</a> to harm no one, <a href='http://");
  client.print(WiFi.localIP());
  client.print("/fm/1'>here</a> to harm red, <a href='http://");
  client.print(WiFi.localIP());
  client.print("/fm/2'>here</a> to harm blue, or <a href='http://");
  client.print(WiFi.localIP());
  client.print("/fm/3'>here</a> to harm both red and blue.");
  client.print("<br>Click <a href='http://");
  client.print(WiFi.localIP());
  client.print("/faster'>here</a> to speed up firing rate or <a href='http://");
  client.print(WiFi.localIP());
  client.print("/slower'>here</a> to slow down firing rate.");
  client.print("</html>");
  client.flush();

  if (change_rate) {
#ifdef DEBUG
    Serial.print("Changing firing rate to ");
    Serial.println(rate_ms);
#endif
    worker.detach();
    worker.attach_ms(rate_ms, run_me);  
  }
  
  // The client will actually be *flushed* then disconnected
  // when the function returns and 'client' object is destroyed (out-of-scope)
  // flush = ensure written data are received by the other side
  Serial.println("Disconnecting from client");
}

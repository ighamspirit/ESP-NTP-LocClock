#include <SoftwareSerial.h>    // built-in
#include <TinyGPS++.h>         // tinyGPSPlus v1.0.3
#include <Adafruit_SSD1306.h>  // Adafruit OLED library v2.5.7
#include <ESP8266WiFi.h>       // UDP packet support
#include <WiFiUdp.h>           // UDP packet support
#include <TimeLib.h>           // Arduino time functions
#include <ESP8266mDNS.h>       // ESP8266 Multicast DNS https://github.com/arduino/esp8266/tree/master/libraries/ESP8266mDNS
#include <ESP8266LLMNR.h>      // ESP8266 LLMNR (Link-Local Multicast Name Resolution)  https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266LLMNR
//
// User Konfigurationen
// Netzwerk Konfifuration
//

// Setze deine WLAN Zugangsdaten ein
#define WIFI_SSID "StardadO"              // replace with your ID
#define WIFI_PASS "leo06cel02nic84den82"  // replace with your password

//Bestimmt dein NTP Port im Normalfall ist das Port 123 nach RFC5905 https://datatracker.ietf.org/doc/html/rfc5905
#define UDP_PORT 123

// weitere Einstellungen:
bool useStaticIP = false;  // Setzen Sie dies auf `false`, um DHCP zu verwenden, StaticIP wenn dann nicht verwendet

// Wenn eine fest IP genutzt werden soll:
IPAddress staticIP(10, 44, 44, 123);  // Statische IP-Adresse
IPAddress gateway(10, 44, 44, 1);     // Gateway-Adresse
IPAddress subnet(255, 255, 255, 0);   // Subnetzmaske
IPAddress dns(10, 44, 44, 1);         // DNS-Server

//Hostname deines Gerätes
String newHostname = "ntp";  // Hostname nach RFC608 https://datatracker.ietf.org/doc/html/rfc608

// Erstellen Sie ein Timezone-Objekt mit der gewünschten Zeitzone (https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
// Beispiel: Central European Time (Berlin) = "CET-1CEST,M3.5.0,M10.5.0/3"
// Beispiel: Coordinated Universal Time (UTC) = "UTC0"
// Weitere Zeitzonen angaben gibt es hier: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char* TZ_INFO = "UTC0";

//GPS Konfig
SoftwareSerial gpsSerial(D5, D6);
TinyGPSPlus gps;

//OLED Konfig
Adafruit_SSD1306 led(128, 64, &Wire);
volatile int syncFlag = 0;

unsigned long lastChange = 0;
bool showLocator = true;  // Startet mit der Anzeige des Locators

// 'Logo_IGH_128x64', 128x64px6
const uint8_t startbild[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x00, 0x07, 0xff, 0xfc, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0xff, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x00, 0x7f, 0xff, 0xff, 0x80, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x03, 0xff, 0xff, 0xff, 0xf0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x07, 0xff, 0xff, 0xff, 0xf8, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xf0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x3f, 0xff, 0x80, 0x1f, 0xc0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x7f, 0xfe, 0x00, 0x0f, 0x80, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x7f, 0xfc, 0x00, 0x03, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x30, 0xf0, 0x31, 0x86, 0x1c, 0x63, 0xe7, 0xc6, 0x3e, 0x33, 0xf8, 0x3e, 0x0c, 0x60, 0x00,
  0x00, 0x31, 0x90, 0x31, 0x8f, 0x1c, 0xe7, 0x46, 0x66, 0x33, 0x30, 0xe0, 0x30, 0x0c, 0x40, 0x00,
  0x00, 0x33, 0x00, 0x31, 0x8f, 0x1c, 0xe7, 0x06, 0x26, 0x33, 0x30, 0xc0, 0x30, 0x04, 0xc0, 0x00,
  0x00, 0x33, 0x30, 0x3f, 0x89, 0x1e, 0xe3, 0xc6, 0x66, 0x37, 0x30, 0xc0, 0x3c, 0x06, 0xc0, 0x00,
  0x00, 0x33, 0x38, 0x3f, 0x8d, 0x9f, 0xe1, 0xe7, 0xc6, 0x3e, 0x30, 0xc0, 0x3c, 0x06, 0xc0, 0x00,
  0x00, 0x33, 0x18, 0x31, 0x9f, 0x9b, 0x60, 0x66, 0x06, 0x36, 0x30, 0xc0, 0x30, 0x03, 0x80, 0x00,
  0x00, 0x31, 0xf8, 0x31, 0x99, 0x9b, 0x67, 0x66, 0x06, 0x33, 0x30, 0xc0, 0x3e, 0x63, 0x8c, 0x00,
  0x00, 0x30, 0xf0, 0x31, 0x90, 0xd8, 0x63, 0xc6, 0x06, 0x33, 0x30, 0xc0, 0x3e, 0x63, 0x88, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0x80, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0xc0, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0xc0, 0x0f, 0xff, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x03, 0xff, 0xc0, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xc0, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xe0, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xe0, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x01, 0xff, 0xf0, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0xff, 0xf0, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0xff, 0xf8, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x7f, 0xfc, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x7f, 0xff, 0x00, 0x0f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x3f, 0xff, 0xe0, 0x7f, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x07, 0xff, 0xff, 0xff, 0xfc, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x03, 0xff, 0xff, 0xff, 0xf8, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x00, 0xff, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x3f, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x1f, 0xfc, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

WiFiUDP UDP;
char packet[255];  // incoming UDP buffer

struct ntpTime_t {    // NTP timestamp is a 64-bit structure:
  uint32_t seconds;   // # seconds since start of epoch (1/1/1900)
  uint32_t fraction;  // 0x00000001 = 2^-32 second ~ 0.2 nS.
} ntpTime;

struct ntpPacket {  // full packet is 48 bytes in length
  uint8_t control;
  uint8_t stratum;
  uint8_t polling;
  int8_t precision;
  int32_t rootDelay;
  uint32_t rootDispersion;
  char refID[4];
  ntpTime_t referenceTime;
  ntpTime_t originateTime;
  ntpTime_t receiveTime;
  ntpTime_t transmitTime;
} ntp;

IRAM_ATTR void ppsHandler() {
  syncFlag = 1;
}

ntpTime_t unixToNTPTime(time_t t) {
  ntpTime_t response;
  response.seconds = htonl(t + 2208988800L);  // ntp starts 1/1/1900; unix starts 1/1/1970
  response.fraction = 0;                      // no fractional seconds in this sketch
  return response;
}

void prepareResponse() {                     // create NTP packet with current time
  ntp.control = (ntp.control & 0x38) + 4;    // change to leap 0 and mode 4(server)
  ntp.stratum = 0x01;                        // we are calling it stratum-1
  ntp.precision = 0x00;                      // one second precision: 2^0 = 1 S
  strncpy(ntp.refID, "GPS", 4);              // reference clock is the GPS service
  ntp.originateTime = ntp.transmitTime;      // time that client transmitted request
  ntp.referenceTime = unixToNTPTime(now());  // time of sync with reference clock
  ntp.receiveTime = ntp.referenceTime;       // time that server received request
  ntp.transmitTime = ntp.referenceTime;      // time that server responded
}

void answerQuery() {
  memcpy(&ntp, &packet, sizeof(ntp));  // copy UDP buffer to NTP struct
  prepareResponse();                   // add time data to NTP struct
  memcpy(&packet, &ntp, sizeof(ntp));  // copy NTP stuct back to UDP buffer
  UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
  UDP.write(packet, sizeof(ntp));  // send NTP response over UDP
  UDP.endPacket();
}

void connectToWiFi() {
  if (useStaticIP) {
    WiFi.config(staticIP, gateway, subnet, dns);
  }
  WiFi.hostname(newHostname.c_str());
  Serial.printf("Connection status: %d\n", WiFi.status());
  Serial.printf("Connecting to %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);  // start Wi-Fi connection attempt
  Serial.printf("Connection status: %d\n", WiFi.status());
  led.clearDisplay();   // erase previous screen
  led.setCursor(0, 0);  // start at top-left
  led.setTextSize(2);   // size 2 is easily readable
  led.print("WiFi");    // show Wi-Fi connnection progress:
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) {  // while trying to connect,
    delay(500);                            // print a dot every 0.5 sec.
    led.print(".");
    Serial.print(".");
    led.display();
  }
  Serial.println(".");
  Serial.printf("\nConnection status: %d\n", WiFi.status());
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(newHostname.c_str())) {
    Serial.println("Error setting up MDNS responder!");
    while (1) { delay(1000); }
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("ntp", "udp", 123);
  // Start LLMNR responder
  LLMNR.begin(newHostname.c_str());
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);  // GPS Data @ 9600 baud
  led.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  led.clearDisplay();
  led.drawBitmap(0, 0, startbild, 128, 64, WHITE);
  led.display();
  delay(2000);              // Zeigt das Bild für 2 Sekunden an
  led.setTextColor(WHITE);  // white on black text
  led.setTextSize(2);
  led.clearDisplay();
  connectToWiFi();      // establish WiFi connection
  UDP.begin(UDP_PORT);  // start listening for UDP packets
  led.clearDisplay();
  led.setCursor(0, 0);
  led.print("Warten auf");
  led.setCursor(0, 16);
  led.print("GPS Daten!");
  led.display();
  attachInterrupt(digitalPinToInterrupt(D7), ppsHandler, RISING);  // enable 1pps GPS time sync
  setenv("TZ", TZ_INFO, 1);
  tzset();
}

void getGPSTime() {
  int h = gps.time.hour();     // get the hour
  int m = gps.time.minute();   // get the minutes
  int s = gps.time.second();   // get the seconds
  int d = gps.date.day();      // get day
  int mo = gps.date.month();   // get month
  int y = gps.date.year();     // get year
  setTime(h, m, s, d, mo, y);  // set the system time
  adjustTime(1);               // and adjust forward 1 second
}

String calculateLocator(double lat, double lon) {
  // Implementieren Sie hier die Umrechnung in das Locatorfeld
  // Beispiel-Implementierung:
  char locator[9];
  lon += 180;
  lat += 90;
  locator[0] = 'A' + (int)(lon / 20);
  locator[1] = 'A' + (int)(lat / 10);
  lon = fmod(lon, 20);
  lat = fmod(lat, 10);
  locator[2] = '0' + (int)(lon / 2);
  locator[3] = '0' + (int)(lat / 1);
  lon = fmod(lon, 2);
  lat = fmod(lat, 1);
  locator[4] = 'A' + (int)(lon * 12);
  locator[5] = 'A' + (int)(lat * 24);
  lon = fmod(lon * 12, 1);
  lat = fmod(lat * 24, 1);
  locator[6] = '0' + (int)(lon * 10);
  locator[7] = '0' + (int)(lat * 10);
  locator[8] = 0;  // Null-Terminator
  return String(locator);
}

void drawWifiStrength(int x, int y) {
  int rssi = WiFi.RSSI();
  int bars = 0;

  // Bestimmen der Anzahl der gefüllten Balken basierend auf der RSSI
  if (rssi > -55) bars = 4;
  else if (rssi > -65) bars = 3;
  else if (rssi > -75) bars = 2;
  else if (rssi > -85) bars = 1;

  // Zeichnen der WLAN-Balken
  for (int i = 0; i < 4; i++) {
    if (i < bars) {
      led.fillRect(x + i * 5, y, 3, 5, WHITE);  // Gefüllter Balken
    } else {
      led.drawRect(x + i * 5, y, 3, 5, WHITE);  // Leerer Balken
    }
  }
}

void drawSatelliteIcon(int x, int y) {
  // Zeichnet den Körper des Satelliten
  led.drawRect(x + 2, y + 2, 4, 4, WHITE);  // Kleinerer Körper

  // Zeichnet die Antennen
  led.drawLine(x + 6, y + 2, x + 8, y, WHITE);      // Obere rechte Antenne
  led.drawLine(x + 6, y + 4, x + 8, y + 6, WHITE);  // Untere rechte Antenne
  led.drawLine(x - 2, y + 6, x, y + 4, WHITE);      // Untere linke Antenne
  led.drawLine(x - 2, y, x, y + 2, WHITE);          // Obere linke Antenne
}

void drawClockIcon(int x, int y) {
  int radius = 7;                                                           // Radius des Uhrensymbols
  led.drawCircle(x + radius, y + radius, radius, WHITE);                    // Zeichnet den Außenkreis der Uhr    // Zeichnet den Stundenzeiger
  led.drawLine(x + radius, y + radius, x + radius, y + radius - 6, WHITE);  // Zeichnet den Minutenzeiger
  led.drawLine(x + radius, y + radius, x + radius + 6, y + radius, WHITE);
}

void drawMapIcon(int x, int y) {
  int width = 14;   // Breite des Karten-Symbols
  int height = 14;  // Höhe des Karten-Symbols

  led.drawRect(x, y, width, height, WHITE);                    // Zeichnet das Rechteck für die Karte
  led.drawLine(x + 2, y + 5, x + width - 2, y + 5, WHITE);     // Zeichnet eine "Straße"
  led.drawLine(x + 2, y + 10, x + width - 2, y + 10, WHITE);   // Zeichnet eine weitere "Straße"
  led.drawLine(x + 5, y + 2, x + 5, y + height - 2, WHITE);    // Zeichnet eine "Straße" quer
  led.drawLine(x + 10, y + 2, x + 10, y + height - 2, WHITE);  // Zeichnet eine weitere "Straße" quer
}

void drawMountainIcon(int x, int y) {
  // Grundlinie des Berges
  led.drawLine(x, y + 6, x + 6, y + 6, WHITE);

  // Großer Berg in der Mitte
  led.drawLine(x + 3, y, x, y + 6, WHITE);
  led.drawLine(x + 3, y, x + 6, y + 6, WHITE);

  // Kleinerer Berg links
  led.drawLine(x + 1, y + 3, x, y + 6, WHITE);
  led.drawLine(x + 1, y + 3, x + 3, y + 6, WHITE);

  // Kleinerer Berg rechts
  led.drawLine(x + 4, y + 3, x + 3, y + 6, WHITE);
  led.drawLine(x + 4, y + 3, x + 6, y + 6, WHITE);
}

void drawNetworkIcon(int x, int y) {
  // Zentraler Punkt oder kleines Symbol
  led.fillCircle(x + 4, y + 4, 1, WHITE);  // Kleiner Kreis in der Mitte

  // Konzentrische Bögen
  led.drawCircle(x + 4, y + 4, 2, WHITE);  // Erster Bogen
  led.drawCircle(x + 4, y + 4, 3, WHITE);  // Zweiter Bogen
  led.drawCircle(x + 4, y + 4, 4, WHITE);  // Dritter Bogen

  // Optional: Linien, um die "Richtung" der Wellen anzudeuten
  led.drawLine(x + 4, y, x + 4, y + 2, WHITE);      // Linie nach oben
  led.drawLine(x + 4, y + 6, x + 4, y + 8, WHITE);  // Linie nach unten
  led.drawLine(x, y + 4, x + 2, y + 4, WHITE);      // Linie nach links
  led.drawLine(x + 6, y + 4, x + 8, y + 4, WHITE);  // Linie nach rechts
}

void drawLatitudeIcon(int x, int y) {
  // Zeichnet einen einfachen Kreis, der einen Globus darstellt
  led.drawCircle(x + 7, y + 7, 7, WHITE);

  // Zeichnet eine horizontale Linie durch den Kreis, die einen Breitenkreis darstellt
  led.drawLine(x, y + 7, x + 14, y + 7, WHITE);
}

void drawLongitudeIcon(int x, int y) {
  // Zeichnet einen einfachen Kreis, der einen Globus darstellt
  led.drawCircle(x + 7, y + 7, 7, WHITE);

  // Zeichnet eine vertikale Linie durch den Kreis, die einen Meridian darstellt
  led.drawLine(x + 7, y, x + 7, y + 14, WHITE);
}

void displayTimeAndLocator() {
  led.clearDisplay();
  led.setCursor(0, 0);

  time_t localTime;
  time(&localTime);                             // Holt die aktuelle Zeit als time_t-Wert
  struct tm* tm_local = localtime(&localTime);  // Wandelt in lokale Zeit um

  int h = tm_local->tm_hour;  // 24-Stunden-Format
  int m = tm_local->tm_min;
  int s = tm_local->tm_sec;
  int day = tm_local->tm_mday;
  int month = tm_local->tm_mon + 1;  // tm_mon ist 0-basiert


  led.setTextSize(2);
  drawClockIcon(0, 0);   // Sie können die Position anpassen
  led.setCursor(20, 0);  // Position anpassen, um genug Platz für das Symbol zu lassen

  if (h < 10) led.print("0");
  led.print(h);
  led.print(":");
  if (m < 10) led.print("0");
  led.print(m);
  led.print(":");
  if (s < 10) led.print("0");
  led.print(s);

  if (showLocator) {
    // Anzeigen des Locators
    drawMapIcon(0, 16);     // Sie können die Position anpassen
    led.setCursor(20, 16);  // Setzen Sie den Cursor für die zweite Zeile
    if (gps.location.isValid()) {
      String locator = calculateLocator(gps.location.lat(), gps.location.lng());
      led.print(locator);
    } else {
      drawMapIcon(0, 16);
      led.setCursor(20, 16);
      led.print("GPS?");
    }

  } else {
    // Anzeigen der Koordinaten
    led.setTextSize(2);
    led.setCursor(0, 16);  // Setzen Sie den Cursor für die zweite Zeile
    if (gps.location.isValid()) {
      drawLatitudeIcon(0, 16);
      led.setCursor(20, 16);
      led.print(gps.location.lat(), 5);  // Breitengrad
      led.setCursor(0, 32);              // Setzen Sie den Cursor für die dritte Zeile
      drawLongitudeIcon(0, 32);
      led.setCursor(20, 32);
      led.print(gps.location.lng(), 5);  // Längengrad
    } else {
      drawMapIcon(0, 16);
      led.setCursor(20, 16);
      led.print("GPS?");
    }
  }

  led.setTextSize(1);
  drawSatelliteIcon(0, 48);
  led.setCursor(20, 48);              // Position anpassen
  led.print(gps.satellites.value());  // Anzahl der Satelliten

  led.setTextSize(1);
  drawMountainIcon(40, 48);
  led.setCursor(55, 48);  // Position anpassen
  if (gps.altitude.isValid()) {
    led.print("");
    int altitudeMeters = (int)gps.altitude.meters();  // Konvertiert in Ganzzahl
    led.print(altitudeMeters);                        // Anzeigen der Höhe in Metern
    led.print("m");
  } else {
    led.print("??m");
  }
  led.setTextSize(1);
  drawWifiStrength(100, 48);  // Position anpassen, wo die Signalstärke angezeigt werden soll

  drawNetworkIcon(0, 56);
  led.setTextSize(1);
  led.setCursor(20, 56);
  led.print(WiFi.localIP());  // Anzahl der Satelliten
  led.display();
}

void feedGPS() {                // feed GPS data into GPS parser
  if (gpsSerial.available()) {  // if a character is ready to read...
    char c = gpsSerial.read();  // read the character
    gps.encode(c);              // and feed the char to the GPS parser
  }
}

void syncWithGPS() {
  if (syncFlag) {             // is it time to synchonize time?
    syncFlag = 0;             // reset the flag, and
    getGPSTime();             // set system time from GPS
    displayTimeAndLocator();  // and show it on OLED display
  }
}

void checkForNTP() {
  if (UDP.parsePacket()) {            // look for a received packet
    int len = UDP.read(packet, 255);  // packet found, so read it into buffer
    if (len) answerQuery();           // send NTP response
  }
}

void loop() {
  feedGPS();      // read data from GPS
  syncWithGPS();  // sync clock every second
  checkForNTP();  // answer any incoming NTP requests

  if (millis() - lastChange > 10000) {  // 10 Sekunden in Millisekunden
    showLocator = !showLocator;         // Wechselt zwischen Locator und Koordinaten
    lastChange = millis();              // Aktualisiert den Zeitpunkt der letzten Änderung
    displayTimeAndLocator();            // Aktualisiert die Anzeige
  }
}

#include <SoftwareSerial.h>    // built-in
#include <TinyGPS++.h>         // tinyGPSPlus v1.0.3
#include <Adafruit_SSD1306.h>  // Adafruit OLED library v2.5.7
#include <ESP8266WiFi.h>       // UDP packet support
#include <WiFiUdp.h>           // UDP packet support
#include <TimeLib.h>           // Arduino time functions

#define WIFI_SSID "StardadO"              // replace with your ID
#define WIFI_PASS "leo06cel02nic84den82"  // replace with your password
#define UDP_PORT 123

SoftwareSerial gpsSerial(D5, D6);
TinyGPSPlus gps;
Adafruit_SSD1306 led(128, 64, &Wire);
volatile int syncFlag = 0;

unsigned long lastChange = 0;
bool showLocator = true;  // Startet mit der Anzeige des Locators

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

bool isSummerTime(int month, int day, int hour) {
  // Sommerzeit beginnt am letzten Sonntag im März
  if (month == 3) {
    return day > 24;  // Vereinfachte Annahme
  }
  // Sommerzeit endet am letzten Sonntag im Oktober
  if (month == 10) {
    return day < 25;  // Vereinfachte Annahme
  }
  return month > 3 && month < 10;
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
  WiFi.begin(WIFI_SSID, WIFI_PASS);        // start Wi-Fi connection attempt
  led.clearDisplay();                      // erase previous screen
  led.setCursor(0, 0);                     // start at top-left
  led.setTextSize(2);                      // size 2 is easily readable
  led.print("WiFi");                       // show Wi-Fi connnection progress:
  while (WiFi.status() != WL_CONNECTED) {  // while trying to connect,
    delay(500);                            // print a dot every 0.5 sec.
    led.print(".");
    led.display();
  }
}

void setup() {
  gpsSerial.begin(9600);                  // GPS Data @ 9600 baud
                                          //  Serial.begin(115200);
  led.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // turn on OLED display
  led.setTextColor(WHITE);                // white on black text
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
}


void getGPSTime() {            // set Arduino clock from GPS
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

void drawSatelliteIcon(int x, int y) {
    // Zeichnet den Körper des Satelliten
    led.drawRect(x + 2, y + 2, 4, 4, WHITE); // Kleinerer Körper

    // Zeichnet die Antennen
    led.drawLine(x + 6, y + 2, x + 8, y, WHITE);       // Obere rechte Antenne
    led.drawLine(x + 6, y + 4, x + 8, y + 6, WHITE);   // Untere rechte Antenne
    led.drawLine(x - 2, y + 6, x, y + 4, WHITE);       // Untere linke Antenne
    led.drawLine(x - 2, y, x, y + 2, WHITE);           // Obere linke Antenne
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
  int h = gps.time.hour();  // 24-Stunden-Format
  int m = gps.time.minute();
  int s = gps.time.second();
  int day = gps.date.day();
  int month = gps.date.month();

  // Überprüfen, ob Sommerzeit ist und entsprechend anpassen
  if (!isSummerTime(month, day, h)) {
    h -= 1;              // Eine Stunde zurück für Winterzeit
    if (h < 0) h += 24;  // Korrektur für Mitternacht
  }

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
      led.print("GPS?");
    }
  }

  led.setTextSize(1);
  drawSatelliteIcon(0, 48);
  led.setCursor(20, 48);              // Position anpassen
  led.print(gps.satellites.value());  // Anzahl der Satelliten

  led.setTextSize(1);
  drawMountainIcon(60, 48);
  led.setCursor(80, 48);  // Position anpassen
  if (gps.altitude.isValid()) {
    led.print("");
    int altitudeMeters = (int)gps.altitude.meters();  // Konvertiert in Ganzzahl
    led.print(altitudeMeters);                        // Anzeigen der Höhe in Metern
    led.print("m");
  } else {
    led.print("??m");
  }
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

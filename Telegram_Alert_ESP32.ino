#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HardwareSerial.h>

const char* WIFI_SSID = "Loading...";
const char* WIFI_PASS = "@3382absd@";

const char* TELEGRAM_BOT_TOKEN = "8051630381:AAEyDF6UGkeFuIFPA6H2OLSWnjTUIZKTYqE";  // from @BotFather
const char* CHAT_ID = "1407137702";                    // your chat id

// Use UART2 => RX=GPIO16, TX=GPIO17
HardwareSerial linkUart(2);

void sendTelegram(const String& text) {
  WiFiClientSecure client;
  client.setInsecure(); // for simplicity; or load Telegram's root CA

  if (!client.connect("api.telegram.org", 443)) return;

  String url = String("/bot") + TELEGRAM_BOT_TOKEN + "/sendMessage";
  String payload = String("chat_id=") + CHAT_ID + "&text=" + urlencode(text);

  // HTTPS POST
  client.println(String("POST ") + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(payload.length());
  client.println();
  client.print(payload);

  // optional: read response
  while (client.connected() || client.available()) {
    if (client.available()) client.read();
  }
}

String urlencode(const String& s) {
  String out;
  const char *hex = "0123456789ABCDEF";
  for (size_t i=0; i<s.length(); i++) {
    char c = s[i];
    if (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~') out += c;
    else if (c==' ') out += '+';
    else {
      out += '%';
      out += hex[(c >> 4) & 0xF];
      out += hex[c & 0xF];
    }
  }
  return out;
}

void setup() {
  Serial.begin(115200);

  // UART2 @ 9600 baud (matches Nano)
  linkUart.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    sendTelegram("ESP32 online ✅");
  } else {
    // still proceed; messages will fail until Wi-Fi connects
  }
}

void loop() {
  static String line;
  while (linkUart.available()) {
    char c = linkUart.read();
    if (c == '\n' || c == '\r') {
      if (line.length() > 0) {
        // Expect lines like: EMF:LOW:123
        if (line.startsWith("EMF:")) {
          sendTelegram("⚡ EMF Alert: " + line);
        }
        else if (line.startsWith("WATER:")) {
          sendTelegram("💧 Water Level Alert: " + line);
        }
        
        
        line = "";
      }
    } else {
      line += c;
      if (line.length() > 120) line = ""; // sanity reset
    }
  }
}

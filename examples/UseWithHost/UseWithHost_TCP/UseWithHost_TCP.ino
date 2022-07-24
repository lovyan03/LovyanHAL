
// #define SETUP_WIFI_SSID_PASSWORD "SSID","PASS"

#include <LovyanHAL.hpp>
#include <lhal/use_with_host/for_mcu.hpp>

#include <WiFi.h>
#include <WiFiServer.h>

WiFiServer tcp_server;
WiFiClient tcp_client;

class TransportStream : public lhal::internal::ITransportLayer
{
private:
  WiFiClient* _st;
public:
  TransportStream(WiFiClient* st) : _st { st } {}

  int read(void) override
  {
    if (_st->available()) { return _st->read(); }
    else { return -1; }
  }

  int write(uint8_t value) override
  {
    return _st->write(value);
  }

  int write(const uint8_t* value, size_t len) override
  {
    auto res = _st->write(value, len);
    _st->flush();
    return res;
  }
};

LHAL hal;
TransportStream st ( &tcp_client );

void setup()
{
  Serial.begin(115200);

  Serial.println("WiFi begin.");

  WiFi.mode(WIFI_MODE_STA);

#if defined ( SETUP_WIFI_SSID_PASSWORD )
  WiFi.begin(SETUP_WIFI_SSID_PASSWORD);
#else
  WiFi.begin();
#endif

  for (int i = 0; WiFi.status() != WL_CONNECTED && i < 100; i++) { delay(100); }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("SmartConfig start.");
    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.beginSmartConfig();

    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
    }
    WiFi.stopSmartConfig();
    WiFi.mode(WIFI_MODE_STA);
  }
  Serial.println(String("IP:") + WiFi.localIP().toString());

  tcp_server.setNoDelay(true);
  tcp_server.begin(lhal::internal::default_tcp_port);
}

void loop()
{
  if (!tcp_client.connected()) {
    tcp_client = tcp_server.available();
  }
  else
  {
    lhal::internal::perform_use_with_host(&hal, &st);
  }
  delay(1);
}

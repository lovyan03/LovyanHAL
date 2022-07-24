#include <LovyanHAL.hpp>
#include <lhal/use_with_host/for_mcu.hpp>

class TransportStream : public lhal::internal::ITransportLayer
{
private:
  Stream* _st;
public:
  TransportStream(Stream* st) : _st { st } {}

  int read(void) override
  {
    return _st->read();
  }

  int write(uint8_t value) override
  {
    return _st->write(value);
  }

  int write(const uint8_t* value, size_t len) override
  {
    return _st->write(value, len);
  }
};

LHAL hal;
TransportStream st ( &Serial );

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  lhal::internal::perform_use_with_host(&hal, &st);

  static uint_fast8_t prev_interval;
  uint_fast8_t interval = millis() >> 12;
  if (prev_interval != interval)
  {
    prev_interval = interval;
    delay(1);
  }
}

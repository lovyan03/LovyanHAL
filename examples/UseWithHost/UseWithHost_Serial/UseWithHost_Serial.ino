#include <LovyanHAL.hpp>
#include <LHAL_terminal.hpp>

class TransportStream : public lhal::internal::ITransportLayer
{
private:
  Stream* _st;
public:
  TransportStream(Stream* st) : _st { st } {}

  int read(void) override
  {
    return (_st->available()) ? _st->read() : -1;
  }

  int write(uint8_t value) override
  {
    return _st->write(value);
  }

  int write(const uint8_t* value, size_t len) override
  {
    _st->flush();
    return _st->write(value, len);
  }
};

LHAL hal;
TransportStream st ( &Serial );

void setup()
{
  Serial.begin( lhal::internal::default_serial_baudrate );
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

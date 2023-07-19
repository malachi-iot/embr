#include <estd/ostream.h>

#include <embr/word.h>

embr::word<32> w;

static estd::arduino_ostream cout(Serial);

void setup()
{
	Serial.begin(115200);
}

void loop()
{
	static unsigned counter = 0;

	cout << F("Hello World: ") << ++counter << F(", ");

	// NOTE: On AVR this one line adds nearly 400 bytes.  Appears to be a direct
	// artifact of 32-bit integer on a 16-bit device.  For 16-bit it adds
	// less than 100 bytes.  That kinda makes sense
	// DEBT: Add << overload so that word can emit directly
	cout << (++w).value();

	cout << estd::endl;

	delay(1000);
}

#include <embr/word.h>

volatile embr::word<32> w;

void setup()
{
}

void loop()
{
	// FIX: Discards volatile qualifier, which I'm sure is an error in some scenarios
	++w;
}

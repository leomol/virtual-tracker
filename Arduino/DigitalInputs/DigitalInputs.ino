/**
 * Setup an Arduino to listen and report changes to digital inputs.
 * 
 * Changes to pins 0 to 63 are reported in single bytes:
 *   bits 1 to 6 indicate the pin number.
 *   bit 7 indicates a positive or a negative change with 0 or 1, respectively.
 *   bit 8 is always set to zero to allow for an extended protocol definition.
 * 
 * @file DigitalInputs.ino
 * @author Leonardo Molina (leonardomt@gmail.com)
 * @date 2016-12-01
 * @version: 0.1.180710
 */

#include "DigitalInput.h"

using namespace bridge;

/// Serial communication baudrate.
const int32_t baudrate = 115200;

/// GPIO to configure as digital inputs.
const int8_t digitalInputPins[] = {16, 17, 18, 19};

/// Digital input array size.
const int8_t nDigitalInputs = sizeof(digitalInputPins);

/// Digital input object array.
DigitalInput** digitalInputs = new DigitalInput*[nDigitalInputs];

/// Arduino library setup.
void setup() {
	// Initialize serial communication.
	Serial.begin(baudrate);
	
	// Setup digital inputs.
	for (int i = 0; i < nDigitalInputs; i++)
		digitalInputs[i] = new DigitalInput(digitalInputPins[i], digitalResponse);
}

/// Arduino library loop.
void loop() {
	for (int i = 0; i < nDigitalInputs; i++)
		digitalInputs[i]->Step();
}

/// Process a response from DigitalInput.
void digitalResponse(DigitalInput* digitalInput, bool state) {
	sendState(digitalInput->GetPin(), state);
}

/// Encode and send state changes.
void sendState(uint8_t pin, bool state) {
	const int8_t lowMask = 64;
	Serial.write(state ? (uint8_t) pin : (uint8_t) (pin + lowMask));
}
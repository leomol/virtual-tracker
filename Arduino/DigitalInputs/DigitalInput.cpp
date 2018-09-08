/**
 * @file DigitalInput.cpp
 * @author Leonardo Molina (leonardomt@gmail.com).
 * @date 2016-12-01
 * @version 0.1.180710
 * 
 * @brief Setup a GPIO as a digital input with pull-up, listen to digital changes, and report.
 */

#include <Arduino.h>
#include "DigitalInput.h"
#include "meta.h"
#include "tools.h"

namespace bridge {
	DigitalInput::DigitalInput(int8_t pin, Function function, FunctionData functionData, Data data) :
	pin(pin),
	function(function),
	functionData(functionData),
	data(data),
	port(BRIDGE_BASEREG(pin)),
	mask(BRIDGE_BITMASK(pin)),
	syncCount(0)
	{
		// In case the pin is disconnected, a pull-up will keep a stable state.
		pinMode(pin, INPUT_PULLUP);
		int interruptId = digitalPinToInterrupt(pin);
		if (interruptId >= 0) {
			// If an interrupt is available, check states using the service routine instead of the step mechanism.
			interruptible = true;
			// Force a report on the first step.
			asyncCount = 1;
			asyncState = BRIDGE_READ(port, mask);
			syncState = !asyncState;
			/* std is not supported in Arduino and a lambda expression cannot be passed as an argument to
			   functions when capturing. As a solution, forward from (*void)(void) to (*void)(uintptr_t) 
			   using a compile-time lookup table (via metaprogramming):
			 */
			attachInterrupt(interruptId, meta::Wrap(OnChange, (Data) this), CHANGE);
		} else {
			// If an interrupt is not available, use the step mechanism.
			interruptible = false;
			// Force a report on the first step.
			syncState = !BRIDGE_READ(port, mask);
		}
	}
	
	DigitalInput::~DigitalInput() {
		// Remove interrupts from this pin.
		detachInterrupt(digitalPinToInterrupt(pin));
	}
	
	void DigitalInput::OnChange(Data data) {
		// Forward static call to object.
		((DigitalInput*) data)->OnChange();
	}
	
	void DigitalInput::OnChange() {
		asyncState = BRIDGE_READ(port, mask);
		asyncCount += 1;
	}
	
	void DigitalInput::Step() {
		bool state;
		uint32_t count;
		uint32_t change;
		// Reconcile count and state from last iteration.
		if (interruptible) {
			/* Disable interrupts briefly to safely copy multi-byte data that would
			   otherwise be at risk of being changed halfways during a copy operation.
			 */
			noInterrupts();
			state = asyncState;
			count = asyncCount;
			interrupts();
			change = count - syncCount;
		} else {
			state = BRIDGE_READ(port, mask);
			if (state == syncState) {
				change = 0;
				count = syncCount;
			} else {
				change = 1;
				count = syncCount + 1;
			}
		}
		// Catch up with pin toggles, one at a time.
		if (change > 0) {
			if (function) {
				for (int c = 0; c < change; c++) {
					syncState = !syncState;
					function(this, syncState);
				}
			} else {
				for (int c = 0; c < change; c++) {
					syncState = !syncState;
					functionData(this, syncState, data);
				}
			}
			syncCount = count;
		}
	}
	
	bool DigitalInput::GetState() {
		return syncState;
	}
	
	int8_t DigitalInput::GetPin() {
		return pin;
	}
}
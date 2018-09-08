/**
 * @file DigitalInput.h
 * @author Leonardo Molina (leonardomt@gmail.com).
 * @date 2016-12-01
 * @version 0.1.180710
 * 
 * @brief Setup a GPIO as a digital input with pull-up, listen to digital changes, and report.
 */

#ifndef BRIDGE_DIGITALINPUT_H
#define BRIDGE_DIGITALINPUT_H

#include <stdint.h>
#include "Stepper.h"

namespace bridge {
	/**
	 * @class DigitalInput
	 * @brief Setup a GPIO as a digital input with pull-up, listen to digital changes, and report.
	 * @details If the GPIO has a hardware interrupt, digital changes are captured when triggered.
	 * The Step method must be called regularly (e.g. from the Arduino loop function) in order to
	 * report changes promptly and capture changes in the absence of interrupts.
	 * When possible, the class uses direct port manipulation to access pin state faster than Arduino's digitalRead.
	 */
	class DigitalInput : public Stepper {
		public:
			/// @typedef User data to include during a callback.
			typedef uintptr_t Data;
			
			/// @typedef Function to invoke during a state change.
			typedef void (*Function) (DigitalInput* digitalInput, bool state);
			
			/// @typedef Function to invoke during a state change, which includes user data.
			typedef void (*FunctionData) (DigitalInput* digitalInput, bool state, Data data);
			
			/// @brief Default constructor.
			DigitalInput() {};
			
			/**
			 * @brief Listen and report pin changes.
			 * @param[in] pin GPIO number.
			 * @param[in] function function to invoke on change.
			 */
			DigitalInput(int8_t pin, Function function) : DigitalInput(pin, function, nullptr, 0) {};
			
			/**
			 * @brief Listen and report pin changes. functionData(this, data) when a change in the pin is detected.
			 * @param[in] pin GPIO number.
			 * @param[in] functionData function to invoke on change along with user data.
			 * @param[in] data user data to include in the callback.
			 */
			DigitalInput(int8_t pin, FunctionData functionData, Data data) : DigitalInput(pin, nullptr, functionData, data) {};
			
			/// @brief Remove any interrupt routines assigned during execution, if any.
			~DigitalInput();
			
			/**
			 * @brief Synchronize "threads", report changes, if any.
			 * @return void
			 */
			void Step() override;
			
			/// @return current digital state of the pin.
			bool GetState();
			
			/// @return pin number.
			int8_t GetPin();
			
		private:
			/**
			 * @brief Listen and report pin changes. This private constructor is used for two construction delegates.
			 * The function not set to nullptr is invoked during a state change.
			 * @param[in] pin GPIO number.
			 * @param[in] function function to invoke on change.
			 * @param[in] functionData function to invoke on change along with user data.
			 * @param[in] data user data to include in the callback.
			 */
			DigitalInput(int8_t pin, Function function, FunctionData functionData, Data data);
			
			volatile uint8_t* port;				///< Hardware address of the pin.
			uint8_t mask;						///< Mask to single out in the hardware address.
			
			Function function;					///< Listener function invoked when pin toggles its state.
			FunctionData functionData;			///< Listener function invoked when pin toggles its state.
			Data data;							///< Data to be passed to function when its invoked.
			
			int8_t pin;							///< Pin number the object is processing.
			static void OnChange(Data data);	///< Call a method from the static context of an interrupt service routine.
			void OnChange();					///< Recipient method to the static analogous with the same name.
			bool interruptible;					///< Whether changes to this pin are captured by an interrupt.
			
			uint32_t asyncCount;				///< Count of state changes in the "interrupt thread".
			bool asyncState;					///< Pin state in the "interrupt thread".
			uint32_t syncCount;					///< Count of state changes in the "main thread".
			bool syncState;						///< Pin state in the "main thread".
	};
}

#endif
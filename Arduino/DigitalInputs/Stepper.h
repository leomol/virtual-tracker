/**
 * @file Stepper.h
 * @author Leonardo Molina (leonardomt@gmail.com).
 * @date 2016-12-01
 * @version 0.1.180710
 * 
 * @brief Abstraction for classes that require time integration.
 */

#ifndef STEPPER_H
#define STEPPER_H

#include <Arduino.h>

namespace bridge {
	/// @brief Stepper abstraction with a pure virtual method Step()
	class Stepper {
		public:
			/** 
			 * @brief Children must implement Step() for time integrations.
			 * @return void
			 */
			virtual void Step() = 0;
	};
}

#endif
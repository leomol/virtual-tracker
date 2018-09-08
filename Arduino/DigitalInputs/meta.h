/**
 * @file meta.h
 * @author Leonardo Molina (leonardomt@gmail.com).
 * @date 2016-12-01
 * @version 0.1.180710
 * 
 * @brief Wrap functions with a parameter into parameterless functions.
 * The method involves using a look up table expanded at compile time.
 */

#ifndef BRIDGE_META_H
#define BRIDGE_META_H

#include <stdint.h>

/// Maximum number of function wrappers to use.
#define BRIDGE_MAX_WRAPPERS 8

namespace bridge {
	namespace meta {
		/** @cond */
		typedef uintptr_t Data;
		typedef void (*FunctionData) (Data data);
		typedef void (*Function) (void);
		
		struct Map {
			Data data;
			FunctionData functionData;
			Function function;
			Map() : data(0), functionData(0), function(0) {}
		};
		
		static int uid = 0;
		static Map map[BRIDGE_MAX_WRAPPERS];

		template<int id>
		inline void Wrapper() {
			return map[id].functionData(map[id].data);
		}

		template<int id>
		inline bool expansion() {
			map[id].function = Wrapper<id>;
			return expansion<id - 1>();
		}

		template<>
		inline bool expansion<0>() {
			map[0].function = Wrapper<0>;
			return true;
		}
		/** @endcond */
		
		/**
		 * @brief Wrap a function1 of type (*void)(Data) to a function2 of type (*void)(void).
		 * Calling function2() results in call to function1(Data). This is useful to invoke methods
		 * from a static context or when 3rd party functions only take parameterless functions. This
		 * is the case of interrupt service routines (ISR) in microcontrollers, where the identity of
		 * the triggering pin is not returned with the interrupt call.
		 * std is not supported in Arduino and a lambda expression cannot be passed as an argument to
		 * functions when capturing. As a solution, forward from (*void)(void) to (*void)(uintptr_t) 
		 * using a compile time lookup table (via metaprogramming).
		 * @param[in] functionData function to invoke along with user data.
		 * @param[in] data user data to include in the callback.
		*/
		inline Function Wrap(FunctionData functionData, Data data) {
			// Expand once using template recursion.
			static bool once = expansion<BRIDGE_MAX_WRAPPERS>();
			map[uid].functionData = functionData;
			map[uid].data = data;
			return map[uid++].function;
		}
	}
}

#endif
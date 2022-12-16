#pragma once

#define RAY_DEBUG
// #define RAY_FULL_TRACE

#ifdef RAY_DEBUG
	
	#define LOG(text) std::cout << text << std::endl
	
	#ifdef RAY_FULL_TRACE
		#define FULL_TRACE(text) LOG(text)
	#else
		#define FULL_TRACE(text)
	#endif
	
#else
	
	#define FULL_TRACE(text)
	
#endif // RAY_DEBUG

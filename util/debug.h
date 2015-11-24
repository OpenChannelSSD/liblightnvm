#ifndef _LIBLIGHTNVM_DEBUG_H_
#define _LIBLIGHTNVM_DEBUG_H_

#include <stdio.h>

// #define LNVM_DEBUG_ENABLED

#define LNVM_ASSERT(c, x, ...) if(!(c)){printf("%s:%s - %d %s" x "\n", \
		__FILE__, __FUNCTION__, __LINE__, strerror(errno), \
		##__VA_ARGS__); fflush(stdout); exit(EXIT_FAILURE); }

#define LNVM_ERROR(x, ...) printf("%s:%s - %d %s" x "\n", __FILE__, \
		__FUNCTION__, __LINE__, strerror(errno), ##__VA_ARGS__); \
		fflush(stdout);

#define LNVM_FATAL(x, ...) printf("%s:%s - %d %s" x "\n", __FILE__, \
		__FUNCTION__, __LINE__, strerror(errno), ##__VA_ARGS__); \
		fflush(stdout);exit(EXIT_FAILURE)

#ifdef LNVM_DEBUG_ENABLED
	#define LNVM_DEBUG(x, ...) printf("%s:%s - %d " x "\n", __FILE__, \
		__FUNCTION__, __LINE__, ##__VA_ARGS__);fflush(stdout);
#else
	#define LNVM_DEBUG(x, ...)
#endif
#endif /* _LIBLIGHTNVM_DEBUG_H_ */

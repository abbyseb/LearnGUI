#ifndef __igtWin32Header_h
#define __igtWin32Header_h

#include "igtConfiguration.h"

#if (defined(_WIN32) || defined(WIN32)) && defined(IGT_BUILD_SHARED_LIBS) 
# ifdef IGT_EXPORTS
#  define IGT_EXPORT __declspec(dllexport)
# else
#  define IGT_EXPORT __declspec(dllimport)
# endif  /* IGT_EXPORT */
#else
/* unix needs nothing */
#define IGT_EXPORT 
#endif


#if (defined(_WIN32) || defined(WIN32)) && defined(IGT_BUILD_SHARED_LIBS) 
# ifdef igtcuda_EXPORTS
#  define igtcuda_EXPORT __declspec(dllexport)
# else
#  define igtcuda_EXPORT __declspec(dllimport)
# endif  /* IGT_EXPORT */
#else
/* unix needs nothing */
#define igtcuda_EXPORT 
#endif

#if (defined(_WIN32) || defined(WIN32)) && defined(IGT_BUILD_SHARED_LIBS) 
# ifdef ITKCudaCommon_EXPORTS
#  define ITKCudaCommon_EXPORT __declspec(dllexport)
# else
#  define ITKCudaCommon_EXPORT __declspec(dllimport)
# endif  /* IGT_EXPORT */
#else
/* unix needs nothing */
#define ITKCudaCommon_EXPORT 
#endif

#endif

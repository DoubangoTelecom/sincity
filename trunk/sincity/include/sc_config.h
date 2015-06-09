#ifndef _SINCITY_CONFIG_H_
#define _SINCITY_CONFIG_H_

#define SC_VERSION_MAJOR 1
#define SC_VERSION_MINOR 54
#define SC_VERSION_MICRO 1305
#if !defined(SC_VERSION_STRING)
#	define SC_VERSION_STRING SC_STRING(SC_CAT(SC_VERSION_MAJOR, .)) SC_STRING(SC_CAT(SC_VERSION_MINOR, .)) SC_STRING(SC_VERSION_MICRO)
#endif

// Windows (XP/Vista/7/CE and Windows Mobile) macro definition.
#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define SC_UNDER_WINDOWS	1
#	if defined(_WIN32_WCE) || defined(UNDER_CE)
#		define SC_UNDER_WINDOWS_CE	1
#		define SC_STDCALL			__cdecl
#	else
#		define SC_STDCALL __stdcall
#	endif
#	if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP)
#		define SC_UNDER_WINDOWS_RT		1
#	endif
#else
#	define SC_STDCALL
#endif
// OS X or iOS
#if defined(__APPLE__)
#	define SC_UNDER_APPLE				1
#   include <TargetConditionals.h>
#endif
#if TARGET_OS_MAC
#	define SC_UNDER_MAC                 1
#endif
#if TARGET_OS_IPHONE
#	define SC_UNDER_IPHONE              1
#endif
#if TARGET_IPHONE_SIMULATOR
#	define SC_UNDER_IPHONE_SIMULATOR	1
#endif

// Disable some well-known warnings
#ifdef _MSC_VER
#if !defined(_CRT_SECURE_NO_WARNINGS)
#		define _CRT_SECURE_NO_WARNINGS
#	endif /* _CRT_SECURE_NO_WARNINGS */
#	define SC_INLINE	_inline
#else
#	define SC_INLINE	inline
#endif

#ifdef __GNUC__
#	define sc_atomic_inc(_ptr_) __sync_fetch_and_add((_ptr_), 1)
#	define sc_atomic_dec(_ptr_) __sync_fetch_and_sub((_ptr_), 1)
#elif defined (_MSC_VER)
#	define sc_atomic_inc(_ptr_) InterlockedIncrement((_ptr_))
#	define sc_atomic_dec(_ptr_) InterlockedDecrement((_ptr_))
#else
#	define sc_atomic_inc(_ptr_) ++(*(_ptr_))
#	define sc_atomic_dec(_ptr_) --(*(_ptr_))
#endif

#if _MSC_VER >= 1400
#	pragma warning( disable : 4290 4800 4251 )
#endif

/* define "TNET_DEPRECATED(func)" macro */
#if defined(__GNUC__)
#	define SC_DEPRECATED(func) __attribute__ ((deprecated)) func
#elif defined(_MSC_VER)
#	define SC_DEPRECATED(func) __declspec(deprecated) func
#else
#	pragma message("WARNING: Deprecated not supported for this compiler")
#	define SC_DEPRECATED(func) func
#endif


#if SC_UNDER_WINDOWS
#	define _WINSOCKAPI_
#	include <windows.h>
#elif SC_UNDER_LINUX
#elif SC_UNDER_MACOS
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

#endif /* _SINCITY_CONFIG_H_ */


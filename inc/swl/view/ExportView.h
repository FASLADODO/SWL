#if !defined(__SWL_VIEW__EXPORT_VIEW_H_)
#define __SWL_VIEW__EXPORT_VIEW_H_ 1


#if defined(WIN32)
#	if defined(_MSC_VER)
#		if defined(SWL_VIEW_EXPORT)
#		    define SWL_VIEW_API __declspec(dllexport)
#			define SWL_VIEW_TEMPLATE_EXTERN
#		else
#		    define SWL_VIEW_API __declspec(dllimport)
#			define SWL_VIEW_TEMPLATE_EXTERN extern
#		endif  // SWL_VIEW_EXPORT
#	else
#		define SWL_VIEW_API
#		define SWL_VIEW_TEMPLATE_EXTERN
#	endif  // _MSC_VER
#elif defined(__MINGW32__)
#	if defined(_USRDLL)
#		if defined(SWL_VIEW_EXPORT)
#			define SWL_VIEW_API __declspec(dllexport)
#		else
#			define SWL_VIEW_API __declspec(dllimport)
#		endif  // SWL_VIEW_EXPORT
#	else
#		define SWL_VIEW_API
#	endif  // _USRDLL
#	define SWL_VIEW_TEMPLATE_EXTERN
#else
#   define SWL_VIEW_API
#	define SWL_VIEW_TEMPLATE_EXTERN
#endif


#endif  // __SWL_VIEW__EXPORT_VIEW_H_

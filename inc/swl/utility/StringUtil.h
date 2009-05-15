#if !defined(__SWL_UTILITY__STRING_UTIL__H_)
#define __SWL_UTILITY__STRING_UTIL__H_ 1


#include "swl/utility/ExportUtility.h"
#include <string>

namespace swl {

//-----------------------------------------------------------------------------------
//	string utility

/**
 *	@brief  Unicode ���ڿ��� multi-byte ���ڿ��� ���� utility class.
 *
 *	unicode string�� multi-byte string����, multi-byte string�� unicode string���� ��ȯ�ϴ� utility API�� �����Ѵ�.
 */
class SWL_UTILITY_API StringUtil
{
public:
	//typedef StringUtil base_type;

public:
	/**
	 *	@brief  unicode string�� multi-byte string���� ��ȯ�ϴ� �Լ�.
	 *	@param  wcstr  [in] ��ȯ�ؾ� �� unicode string.
	 *	@return  unicode string�� ��ȯ�� multi-byte string�� ��ȯ.
	 */
	static std::string wcs2mbs(const std::wstring &wcstr);
	/**
	 *	@brief  multi-byte string�� unicode string���� ��ȯ�ϴ� �Լ�.
	 *	@param  mbstr  [in] ��ȯ�ؾ� �� multi-byte string.
	 *	@return  multi-byte string�� ��ȯ�� unicode string�� ��ȯ.
	 */
	static std::wstring mbs2wcs(const std::string &mbstr);
};

}  // namespace swl


#endif  // __SWL_UTILITY__STRING_UTIL__H_

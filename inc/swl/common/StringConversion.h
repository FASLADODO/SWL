#if !defined(__SWL_COMMON__STRING_CONVERSION__H_)
#define __SWL_COMMON__STRING_CONVERSION__H_ 1


#include "swl/common/ExportCommon.h"
#include <string>

namespace swl {

//-----------------------------------------------------------------------------------
//	string conversion

/**
 *	@brief  Unicode ���ڿ��� multi-byte ���ڿ��� ���� conversion class.
 *
 *	unicode string�� multi-byte string����, multi-byte string�� unicode string���� ��ȯ�ϴ� API�� �����Ѵ�.
 */
class SWL_COMMON_API StringConversion
{
public:
	//typedef StringConversion base_type;

public:
	/**
	 *	@brief  unicode string�� multi-byte string���� ��ȯ�ϴ� �Լ�.
	 *	@param[in]  wcstr  ��ȯ�ؾ� �� unicode string.
	 *	@return  unicode string�� ��ȯ�� multi-byte string�� ��ȯ.
	 */
	static std::string wcs2mbs(const std::wstring &wcstr);
	/**
	 *	@brief  multi-byte string�� unicode string���� ��ȯ�ϴ� �Լ�.
	 *	@param[in]  mbstr  ��ȯ�ؾ� �� multi-byte string.
	 *	@return  multi-byte string�� ��ȯ�� unicode string�� ��ȯ.
	 */
	static std::wstring mbs2wcs(const std::string &mbstr);
};

}  // namespace swl


#endif  // __SWL_COMMON__STRING_CONVERSION__H_

#if !defined(__SWL_COMMON__LOG_EXCEPTION__H_)
#define __SWL_COMMON__LOG_EXCEPTION__H_ 1


#include "swl/common/ExportCommon.h"
#include <string>
#include <exception>


#if !defined(__FUNCTION__)
#if defined(UNICODE) || defined(_UNICODE)
#define __FUNCTION__ L""
#else
#define __FUNCTION__ ""
#endif
#endif


namespace swl {

//-----------------------------------------------------------------------------------
//	exception for log

/**
 *	@brief  application ������ �߻��ϴ� exception�� ���� class.
 *
 *	�� class�� C++ ǥ�� exception class�� std::exception���κ��� ��ӵǾ�����
 *	catch ������ �̿��Ͽ� exception handler�� �ۼ��ϸ� �ȴ�.
 *
 *	������ exception�� catch�Ͽ��� ���, �Ʒ��� ���� ������ �߻��� exception���κ��� �� �� �ִ�.
 *		- level
 *		- message
 *		- �߻���Ų ������ ���
 *		- �߻���Ų ���� �̸�
 *		- �� ��ȣ
 *		- class �̸�
 *		- �Լ� �̸�
 *
 *	exception level�� ��� �⺻������ 7���� �������� ����������.
 */
class SWL_COMMON_API LogException: public std::exception
{
public:
	/**
	 *	@brief  base class�� type definition.
	 */
	typedef std::exception base_type;

public:
	/**
	 *	@brief  �⺻ log level.
	 *
	 *	class�� �����Ǿ� �ִ� �⺻ log level�� 7�ܰ�� 0���� 10���� level ���� ������.
	 *		- L_DEBUG = 0
	 *		- L_TRACE = 2
	 *		- L_INFO = 4
	 *		- L_WARN = 6
	 *		- L_ERROR = 8
	 *		- L_ASSERT = 8
	 *		- L_FATAL = 10
	 */
	enum { L_DEBUG = 0, L_TRACE = 2, L_INFO = 4, L_WARN = 6, L_ERROR = 8, L_ASSERT = 8, L_FATAL = 10 };

public:
	/**
	 *	@brief  [ctor] contructor.
	 *	@param[in]  level  exception�� log level ����.
	 *	@param[in]  message  exception message�� ����Ǵ� ��.
	 *	@param[in]  filePath  exception�� �߻��� ������ ��ü ���.
	 *	@param[in]  lineNo  exception�� �߻��� ������ �� ��ȣ.
	 *	@param[in]  methodName  exception�� �߻��� �Լ� �̸�. e.g.) class_name::method_name()�� ���¸� ���ϰ� �־�� �Ѵ�.
	 *
	 *	exception �߻��� �����Ǵ� ���� ���ڰ���κ��� log�� ���� �ʿ��� ������ �̾Ƴ��� �����Ѵ�.
	 */
	LogException(const unsigned int level, const std::wstring &message, const std::wstring &filePath, const long lineNo, const std::wstring &methodName);
	LogException(const unsigned int level, const std::wstring &message, const std::string &filePath, const long lineNo, const std::string &methodName);
	LogException(const unsigned int level, const std::string &message, const std::string &filePath, const long lineNo, const std::string &methodName);
	/**
	 *	@brief  [dtor] default destructor.
	 *
	 *	�ش� class�κ��� �ڽ� class �Ļ��� �����ϵ��� virtual�� ����Ǿ� �ִ�.
	 */
	virtual ~LogException();

public:
	/**
	 *	@brief  �߻��� exception�� log level ���� ��ȯ.
	 *	@return  log level ���� unsigned int �� ������ ��ȯ.
	 */
#if defined(UNICODE) || defined(_UNICODE)
	unsigned int getLevel() const  {  return level_;  }
#else
	unsigned int getLevel() const  {  return level_;  }
#endif

	/**
	 *	@brief  �߻��� exception�� log message�� ��ȯ.
	 *	@return  log message�� ��ȯ.
	 *
	 *	unicode�� ����ϴ� ��� std::wstring����, �׷��� ���� ��� std::string ��ü�� ��ȯ�Ѵ�.
	 */
#if defined(UNICODE) || defined(_UNICODE)
	const std::wstring & getMessage() const  {  return message_;  }
#else
	const std::string & getMessage() const  {  return message_;  }
#endif

	/**
	 *	@brief  exception�� �߻���Ų ������ ��ü ���.
	 *	@return  exception�� �߻��� ������ ��ü ��θ� ��ȯ.
	 *
	 *	unicode�� ����ϴ� ��� std::wstring����, �׷��� ���� ��� std::string ��ü�� ��ȯ�Ѵ�.
	 */
#if defined(UNICODE) || defined(_UNICODE)
	std::wstring getFilePath() const  {  return filePath_;  }
#else
	std::string getFilePath() const  {  return filePath_;  }
#endif

	/**
	 *	@brief  exception�� �߻���Ų ���� �̸�.
	 *	@return  exception�� �߻��� ������ �̸��� ��ȯ.
	 *
	 *	unicode�� ����ϴ� ��� std::wstring����, �׷��� ���� ��� std::string ��ü�� ��ȯ�Ѵ�.
	 */
#if defined(UNICODE) || defined(_UNICODE)
	std::wstring getFileName() const;
#else
	std::string getFileName() const;
#endif

	/**
	 *	@brief  exception�� �߻���Ų ������ �� ��ȣ.
	 *	@return  exception�� �߻��� ������ �� ��ȣ�� ��ȯ.
	 */
#if defined(UNICODE) || defined(_UNICODE)
	long getLineNumber() const  {  return lineNo_;  }
#else
	long getLineNumber() const  {  return lineNo_;  }
#endif

	/**
	 *	@brief  exception�� �߻���Ų class�� �̸�.
	 *	@return  exception�� �߻��� class�� �̸��� ��ȯ.
	 *
	 *	unicode�� ����ϴ� ��� std::wstring����, �׷��� ���� ��� std::string ��ü�� ��ȯ�Ѵ�.
	 */
#if defined(UNICODE) || defined(_UNICODE)
	std::wstring getClassName() const;
#else
	std::string getClassName() const;
#endif

	/**
	 *	@brief  exception�� �߻���Ų �Լ��� �̸�.
	 *	@return  exception�� �߻��� �Լ��� �̸��� ��ȯ.
	 *
	 *	unicode�� ����ϴ� ��� std::wstring����, �׷��� ���� ��� std::string ��ü�� ��ȯ�Ѵ�.
	 */
#if defined(UNICODE) || defined(_UNICODE)
	std::wstring getMethodName() const;
#else
	std::string getMethodName() const;
#endif

private:
	const unsigned int level_;
#if defined(UNICODE) || defined(_UNICODE)
	const std::wstring message_;
#else
	const std::string message_;
#endif

#if defined(UNICODE) || defined(_UNICODE)
	const std::wstring filePath_;
#else
	const std::string filePath_;
#endif
	const long lineNo_;
#if defined(UNICODE) || defined(_UNICODE)
	const std::wstring methodName_;
#else
	const std::string methodName_;
#endif
};

}  // namespace swl


#endif  //  __SWL_COMMON__LOG_EXCEPTION__H_

#if !defined(__SWL_UTIL__TCP_SOCKET_CLIENT__H_)
#define __SWL_UTIL__TCP_SOCKET_CLIENT__H_ 1


#include "swl/util/ExportUtil.h"
#include <boost/asio.hpp>
#include <string>
#include <vector>


namespace swl {

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  ���������� TCP socket ����� �����ϴ� client class.
 *
 *	TCP socket ����� ���� message�� �ۼ����ϱ� ���� send() �Լ��� receive() �Լ��� ȣ���ϸ� �ȴ�.
 *	TCP socket ����� �����ϴ� �������� ������ �Ʒ��� ����.
 *		- TcpSocketClient ��ü ����
 *		- connect() �Լ��� �̿��Ͽ� TCP server�� ����
 *		- send() and/or receive() �Լ��� �̿��� message �ۼ���
 *		- �۾��� �����ٸ�, disconnect() �Լ��� ȣ���Ͽ� ���� ����
 *		- TcpSocketClient ��ü �Ҹ�
 *
 *	synchronous I/O�� ����Ͽ� �ۼ����� �����Ѵ�.
 */
class SWL_UTIL_API TcpSocketClient
{
public:
	//typedef TcpSocketClient base_type;

public:
	/**
	 *	@brief  [ctor] contructor.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket ����� ���� �ʿ��� �������� �ʱ�ȭ�Ѵ�.
	 */
	TcpSocketClient(boost::asio::io_service &ioService);
	/**
	 *	@brief  [dtor] default destructor.
	 *
	 *	TCP socket ����� �����ϱ� ���� ������ �����Ѵ�.
	 *	��� channel�� ���� �ִ� ��� disconnect() �Լ��� ȣ���Ͽ� �̸� �ݴ´�.
	 */
	~TcpSocketClient();

public:
	/**
	 *	@brief  ������ host �̸��� service �̸��� �̿��� TCP socket server�� channel�� ����.
	 *	@param[in]  hostName  TCP socket server�� host �̸�.
	 *	@param[in]  serviceName  TCP socket server�� service �̸�.
	 *	@return  TCP socket channel�� ���������� ����Ǿ��ٸ� true ��ȯ.
	 *
	 *	���ڷ� �Ѱ��� host �̸��� service �̸��� �̿��Ͽ� TCP socket channel�� �����ϰ�
	 *
	 *	host �̸��� IP address�̳� domain �̸����� ������ �� �ִ�.
	 *		- "abc.com"
	 *		- "100.110.120.130"
	 *	service �̸��� �̳� port ��ȣ�� ������ �� �ִ�.
	 *		- "http" or "daytime"
	 *		- "80"
	 */
#if defined(_UNICODE) || defined(UNICODE)
	bool connect(const std::wstring &hostName, const std::wstring &serviceName);
#else
	bool connect(const std::string &hostName, const std::string &serviceName);
#endif
	/**
	 *	@brief  TCP socket ��� channel�� ������ ����.
	 *
	 *	TCP socket ��� channel�� ������ ����, ��� ���� resource�� ��ȯ�Ѵ�.
	 */
	void disconnect();

	/**
	 *	@brief  TCP socket ����� ���� ���¿� �ִ��� Ȯ��.
	 *	@return  TCP socket ��� channel�� ���� �����̸� true ��ȯ.
	 *
	 *	TCP socket ��� channel�� ���� ���¸� ��ȯ�Ѵ�.
	 */
	bool isConnected() const  {  return isActive_;  }

	/**
	 *	@brief  ������ message�� ����� TCP socket ��� channel�� ���� ����.
	 *	@param[in]  msg  ������ message�� �����ϴ� pointer.
	 *	@param[in]  len  ������ message ����.
	 *	@throw  std::runtime_error  �۽� operation ���� error�� �߻�.
	 *	@return  ������ �۽ŵ� message�� ���̸� ��ȯ. ���ڷ� ������ len���� �۰ų� ����.
	 *
	 *	��û�� message�� TCP socket ����� ���� �����Ѵ�.
	 *	�۽ŵ� message�� ���̴� ���ڷ� �־��� ���̺��� �۰ų� ����
	 *	synchronous I/O�� ���� message�� �����Ѵ�.
	 */
	size_t send(const unsigned char *msg, const size_t len);
	/**
	 *	@brief  ����� TCP socket ��� channel�� ���� message�� ����.
	 *	@param[out]  msg  ���ŵ� message�� ������ pointer.
	 *	@param[in]  len  synchronous I/O�� ���� ������ message�� ������ buffer�� ũ�⸦ ����.
	 *	@throw  std::runtime_error  ���� operation ���� error�� �߻�.
	 *	@return  ������ ���ŵ� message�� ���̸� ��ȯ. ���ڷ� ������ len���� �۰ų� ����.
	 *
	 *	TCP socket ����� ���� ���ŵǴ� message�� ���ڷ� ������ pointer�� ��ü�� �����Ѵ�.
	 *	���ŵ� message�� ���̴� ���ڷ� �־��� ���̺��� �۰ų� ����
	 *	synchronous I/O�� ���� message�� �����Ѵ�.
	 */
	size_t receive(unsigned char *msg, const size_t len);

private:
	boost::asio::ip::tcp::socket socket_;

	bool isActive_;
};

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  �񵿱������� TCP socket ����� �����ϴ� client class.
 *
 *	TCP socket ����� ���� message�� �ۼ����ϱ� ���� send() �Լ��� receive() �Լ��� ȣ���ϸ� �ȴ�.
 *	TCP socket ����� �����ϴ� �������� ������ �Ʒ��� ����.
 *		- AsyncTcpSocketClient ��ü ����
 *		- connect() �Լ��� �̿��Ͽ� TCP server�� ����
 *		- send() and/or receive() �Լ��� �̿��� message �ۼ���
 *		- �۾��� �����ٸ�, disconnect() �Լ��� ȣ���Ͽ� ���� ����
 *		- AsyncTcpSocketClient ��ü �Ҹ�
 *
 *	asynchronous I/O�� ����Ͽ� �ۼ����� �����Ѵ�.
 */
class SWL_UTIL_API AsyncTcpSocketClient
{
public:
	//typedef AsyncTcpSocketClient base_type;

public:
	/**
	 *	@brief  [ctor] contructor.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket ����� ���� �ʿ��� �������� �ʱ�ȭ�Ѵ�.
	 */
	AsyncTcpSocketClient(boost::asio::io_service &ioService);
	/**
	 *	@brief  [dtor] default destructor.
	 *
	 *	TCP socket ����� �����ϱ� ���� ������ �����Ѵ�.
	 *	��� channel�� ���� �ִ� ��� disconnect() �Լ��� ȣ���Ͽ� �̸� �ݴ´�.
	 */
	~AsyncTcpSocketClient();

public:
	/**
	 *	@brief  ������ host �̸��� service �̸��� �̿��� TCP socket server�� channel�� ����.
	 *	@param[in]  hostName  TCP socket server�� host �̸�.
	 *	@param[in]  serviceName  TCP socket server�� service �̸�.
	 *	@return  TCP socket channel�� ���������� ����Ǿ��ٸ� true ��ȯ.
	 *
	 *	���ڷ� �Ѱ��� host �̸��� service �̸��� �̿��Ͽ� TCP socket channel�� �����ϰ�
	 *
	 *	host �̸��� IP address�̳� domain �̸����� ������ �� �ִ�.
	 *		- "abc.com"
	 *		- "100.110.120.130"
	 *	service �̸��� �̳� port ��ȣ�� ������ �� �ִ�.
	 *		- "http" or "daytime"
	 *		- "80"
	 */
#if defined(_UNICODE) || defined(UNICODE)
	bool connect(const std::wstring &hostName, const std::wstring &serviceName);
#else
	bool connect(const std::string &hostName, const std::string &serviceName);
#endif
	/**
	 *	@brief  TCP socket ��� channel�� ������ ����.
	 *	@throw  std::runtime_error  TCP socket�� close �������� error�� �߻�.
	 *
	 *	TCP socket ��� channel�� ������ ����, ��� ���� resource�� ��ȯ�Ѵ�.
	 */
	void disconnect();

	/**
	 *	@brief  TCP socket ����� ���� ���¿� �ִ��� Ȯ��.
	 *	@return  TCP socket ��� channel�� ���� �����̸� true ��ȯ.
	 *
	 *	TCP socket ��� channel�� ���� ���¸� ��ȯ�Ѵ�.
	 */
	bool isConnected() const  {  return isActive_;  }

	/**
	 *	@brief  ������ message�� ����� TCP socket ��� channel�� ���� ����.
	 *	@param[in]  msg  ������ message�� �����ϴ� pointer.
	 *	@param[in]  len  ������ message ����.
	 *	@return  ������ �۽ŵ� message�� ���̸� ��ȯ. ���ڷ� ������ len���� �۰ų� ����.
	 *
	 *	��û�� message�� TCP socket ����� ���� �����Ѵ�.
	 *	�۽ŵ� message�� ���̴� ���ڷ� �־��� ���̺��� �۰ų� ����
	 *	asynchronous I/O�� ���� message�� �����Ѵ�.
	 */
	void send(const unsigned char *msg, const size_t len);
	/**
	 *	@brief  ����� TCP socket ��� channel�� ���� message�� ����.
	 *	@param[out]  msg  ���ŵ� message�� ������ pointer.
	 *	@param[in]  len  synchronous I/O�� ���� ������ message�� ������ buffer�� ũ�⸦ ����.
	 *	@return  ������ ���ŵ� message�� ���̸� ��ȯ. ���ڷ� ������ len���� �۰ų� ����.
	 *
	 *	TCP socket ����� ���� ���ŵǴ� message�� ���ڷ� ������ pointer�� ��ü�� �����Ѵ�.
	 *	���ŵ� message�� ���̴� ���ڷ� �־��� ���̺��� �۰ų� ����
	 *	asynchronous I/O�� ���� message�� �����Ѵ�.
	 */
	void receive(const size_t len);

	/**
	 *	@brief  ���ŵ� message�� ��ȯ.
	 *	@param[out]  buf  message ���� ��û �� ���ŵ� message�� ������ buffer.
	 *
	 *	message ���� ��û �� server�κ��� ���ŵ� message�� ������ �����´�.
	 *	buf �ȿ� �ִ� ���� data�� ��� �����ȴ�.
	 */
	void getReceiveMessage(std::vector<unsigned char> &buf) const;

	/**
	 *	@brief  ���� ���� I/O �۾��� ���.
	 *	@throw  std::runtime_error  �ۼ��� operation�� ����ϴ� �������� error�� �߻�.
	 *
	 *	asynchronous I/O�� ���� ���� ���� �ۼ��� operation�� ����Ѵ�.
	 */
	void cancelIo();

	/**
	 *	@brief  server ���� ��û�� �Ϸ�Ǿ����� Ȯ��.
	 *	@return  ��û�� server ������ �Ϸ�Ǿ��ٸ� true�� ��ȯ.
	 *
	 *	server ���� ��û�� ���������� �Ϸ�Ǿ������� Ȯ���Ѵ�.
	 */
	bool isConnectDone() const  {  return isConnectDone_;  }
	/**
	 *	@brief  message �۽��� �Ϸ�Ǿ����� Ȯ��.
	 *	@return  ��û�� message �۽��� �Ϸ�Ǿ��ٸ� true�� ��ȯ.
	 *
	 *	message �۽� ��û�� ���������� �Ϸ�Ǿ������� Ȯ���Ѵ�.
	 */
	bool isSendDone() const  {  return isSendDone_;  }
	/**
	 *	@brief  message ������ �Ϸ�Ǿ����� Ȯ��.
	 *	@return  ��û�� message ������ �Ϸ�Ǿ��ٸ� true�� ��ȯ.
	 *
	 *	message ���� ��û�� ���������� �Ϸ�Ǿ������� Ȯ���Ѵ�.
	 */
	bool isReceiveDone() const  {  return isReceiveDone_;  }

private:
	void completeConnecting(const boost::system::error_code &ec);
	void completeSending(const boost::system::error_code &ec);
	void completeReceiving(const boost::system::error_code &ec, size_t bytesTransferred);

	void doCloseOperation(const boost::system::error_code &ec);
	void doCancelOperation(const boost::system::error_code &ec);

private:
	boost::asio::ip::tcp::socket socket_;

	bool isActive_;

	std::vector<unsigned char> sendMsg_;
	std::vector<unsigned char> receiveMsg_;

	bool isConnectDone_;
	bool isSendDone_;
	bool isReceiveDone_;
};

}  // namespace swl


#endif  // __SWL_UTIL__TCP_SOCKET_CLIENT__H_

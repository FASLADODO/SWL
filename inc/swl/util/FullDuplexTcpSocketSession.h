#if !defined(__SWL_UTIL__FULL_DUPLEX_TCP_SOCKET_SESSION__H_)
#define __SWL_UTIL__FULL_DUPLEX_TCP_SOCKET_SESSION__H_ 1


#include "swl/util/ExportUtil.h"
#include "swl/util/GuardedBuffer.h"
#include <boost/asio.hpp>


namespace swl {

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  full duplex mode�� �̿��Ͽ� TCP socket ����� �����ϴ� server session class.
 *
 *	���������� asynchronous I/O�� ����ϰ� �����Ƿ� �� class�� ����ϴ� S/W�� �δ��� ���� �ִ� ������ �ִ�.
 *
 *	TCP socket ����� ���� message�� ������ ��쿡�� send() �Լ��� �ش� ������ ���� ȣ���ϸ� �ǰ�,
 *	������ ��쿡�� receive() �Լ��� ȣ���ϸ� �ȴ�.
 */
class SWL_UTIL_API FullDuplexTcpSocketSession
{
public:
	//typedef FullDuplexTcpSocketSession base_type;

public:
	/**
	 *	@brief  [ctor] contructor.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket ��� session�� ���� �ʿ��� �������� �ʱ�ȭ�Ѵ�.
	 */
	FullDuplexTcpSocketSession(boost::asio::ip::tcp::socket &socket);
	/**
	 *	@brief  [dtor] virtual default destructor.
	 *
	 *	TCP socket ��� session�� �����ϱ� ���� �ʿ��� ������ �����Ѵ�.
	 *	��� channel�� ���� �ִ� ��� close() �Լ��� ȣ���Ͽ� �̸� �ݴ´�.
	 */
	virtual ~FullDuplexTcpSocketSession();

public:
	/**
	 *	@brief  TCP socket ����� ����.
	 *	@throw  LogException  TCP socket�� close �������� error�� �߻�.
	 *
	 *	TCP socket ����� ���� �����Ͽ��� channel�� ����, ����ϰ� �ִ� resource�� ��ȯ�Ѵ�.
	 */
	void close();

	/**
	 *	@brief  TCP socket ����� Ȱ��ȭ�Ǿ� �ִ��� Ȯ��.
	 *	@return  TCP socket ��� channel�� Ȱ��ȭ�Ǿ� �ִٸ� true ��ȯ.
	 *
	 *	TCP socket ��� channel�� Ȱ��ȭ ���¸� ��ȯ�Ѵ�.
	 */
	bool isActive() const  {  return isActive_;  }

	/**
	 *	@brief  ������ message�� ����� TCP socket ��� channel�� ���� ����.
	 *	@param[in]  msg  ������ message�� �����ϴ� pointer.
	 *	@param[in]  len  ������ message ����.
	 *
	 *	��û�� message�� TCP socket ����� ���� �����Ѵ�.
	 *	asynchronous I/O�� ���� message�� �����Ѵ�.
	 */
	void send(const unsigned char *msg, const size_t len);
	/**
	 *	@brief  ����� TCP socket ��� channel�� ���� message�� ����.
	 *	@param[out]  msg  ���ŵ� message�� ������ pointer.
	 *	@param[in]  len  asynchronous I/O�� ���� ������ message�� ������ buffer�� ũ�⸦ ����.
	 *	@return  ������ ���ŵ� message�� ���̸� ��ȯ. ���ڷ� ������ len���� �۰ų� ����.
	 *
	 *	TCP socket ����� ���� ���ŵǴ� message�� ���ڷ� ������ pointer�� ��ü�� �����Ѵ�.
	 *	asynchronous I/O�� ���� message�� �����Ѵ�.
	 */
	size_t receive(unsigned char *msg, const size_t len);

	/**
	 *	@brief  ���� ���� I/O �۾��� ���.
	 *	@throw  LogException  �ۼ��� operation�� ����ϴ� �������� error�� �߻�.
	 *
	 *	asynchronous I/O�� ���� ���� ���� �ۼ��� operation�� ����Ѵ�.
	 */
	void cancelIo();

	/**
	 *	@brief  �۽� buffer�� ����� message�� ������ ����.
	 *
	 *	�۽� buffer�� ����Ǿ� �ִ� message�� asynchronous I/O�� ���� �۽��Ѵ�.
	 */
	virtual void startSending();
	/**
	 *	@brief  TCP socket ��� channel�� ���� ������ message�� ���� buffer�� ���� ����.
	 *
	 *	TCP socket ����� ���ŵǴ� message�� asynchronous I/O�� �̿��Ͽ� �����ϱ� �����Ѵ�.
	 */
	virtual void startReceiving();

	/**
	 *	@brief  TCP socket ����� �۽� buffer�� ���.
	 *
	 *	���۵��� ���� �۽� buffer�� ��� message�� �����Ѵ�.
	 *	������ �۽� message�� ������ �������� �� �� �����Ƿ� ����ġ ���� error�� �߻���ų �� �ִ�.
	 */
	void clearSendBuffer();
	/**
	 *	@brief  TCP socket ����� ���� buffer�� ���.
	 *
	 *	TCP socket ��� channel�� ���ŵ� ���� buffer�� ��� message�� �����Ѵ�.
	 *	������ ���� message�� ������ �������� �� �� �����Ƿ� ����ġ ���� error�� �߻���ų �� �ִ�.
	 */
	void clearReceiveBuffer();

	/**
	 *	@brief  TCP socket ��� channel�� �۽� buffer�� ��� �ִ����� Ȯ��.
	 *	@return  �۽� buffer�� ��� �ִٸ� true�� ��ȯ.
	 *
	 *	TCP socket ����� ���� ������ message�� �۽� buffer�� ��� �ִ��� ���θ� ��ȯ�Ѵ�.
	 */
	bool isSendBufferEmpty() const;
	/**
	 *	@brief  TCP socket ��� channel�� ���� buffer�� ��� �ִ����� Ȯ��.
	 *	@return  ���� buffer�� ��� �ִٸ� true�� ��ȯ.
	 *
	 *	TCP socket ����� ���� ���ŵ� message�� ���� buffer�� ��� �ִ��� ���θ� ��ȯ�Ѵ�.
	 */
	bool isReceiveBufferEmpty() const;

	/**
	 *	@brief  TCP socket ����� ���� �۽��� message�� ���̸� ��ȯ.
	 *	@return  �۽� message�� ���̸� ��ȯ.
	 *
	 *	TCP socket ����� ���� ������ message�� �����ϰ� �ִ� �۽� buffer�� ���̸� ��ȯ�Ѵ�.
	 */
	size_t getSendBufferSize() const;
	/**
	 *	@brief  TCP socket ����� ���� ���ŵ� message�� ���̸� ��ȯ.
	 *	@return  ���ŵ� message�� ���̸� ��ȯ.
	 *
	 *	TCP socket ����� ���� ���ŵ� message�� �����ϰ� �ִ� ���� buffer�� ���̸� ��ȯ�Ѵ�.
	 */
	size_t getReceiveBufferSize() const;

protected:
	/**
	 *	@brief  �۽� ��û�� message�� ������ �Ϸ�� ��� ȣ��Ǵ� completion routine.
	 *	@param[in]  ec  message�� �����ϴ� �������� �߻��� ������ error code.
	 *	@throw  LogException  TCP socket�� close �������� error�� �߻�.
	 *
	 *	asynchronous I/O�� �̿��Ͽ� �۽� ��û�� message�� ������ �Ϸ�Ǿ��� �� system�� ���� ȣ��Ǵ� completion routine�̴�.
	 *	startSending() �Լ� ������ asynchronous �۽� ��û�� �ϸ鼭 �ش� �Լ��� completion routine���� ������ �־�� �Ѵ�.
	 */
	virtual void completeSending(const boost::system::error_code &ec);
	/**
	 *	@brief  TCP socket ��� channel�� ���� ���ŵ� message�� �ִ� ��� ȣ��Ǵ� completion routine.
	 *	@param[in]  ec  message�� �����ϴ� �������� �߻��� ������ error code.
	 *	@param[in]  bytesTransferred  ���ŵ� message�� ����.
	 *	@throw  LogException  TCP socket�� close �������� error�� �߻�.
	 *
	 *	asynchronous I/O�� ���� message�� ���ŵǴ� ��� system�� ���� ȣ��Ǵ� completion routine�̴�.
	 *	startReceiving() �Լ� ������ asynchronous ���� ��û�� �ϸ鼭 �ش� �Լ��� completion routine���� ������ �־�� �Ѵ�.
	 */
	virtual void completeReceiving(const boost::system::error_code &ec, size_t bytesTransferred);

private:
	void doSendOperation(const unsigned char *msg, const size_t len);
	void doCloseOperation(const boost::system::error_code &ec);
	void doCancelOperation(const boost::system::error_code &ec);

private:
	static const size_t MAX_SEND_LENGTH_ = 512;
	static const size_t MAX_RECEIVE_LENGTH_ = 512;

	boost::asio::ip::tcp::socket &socket_;

	bool isActive_;

	GuardedByteBuffer sendBuffer_;
	GuardedByteBuffer receiveBuffer_;
	GuardedByteBuffer::value_type sendMsg_[MAX_SEND_LENGTH_];
	GuardedByteBuffer::value_type receiveMsg_[MAX_RECEIVE_LENGTH_];
	size_t sentMsgLength_;
};

}  // namespace swl


#endif  // __SWL_UTIL__FULL_DUPLEX_TCP_SOCKET_SESSION__H_

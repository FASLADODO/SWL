#if !defined(__SWL_UTIL__HALF_DUPLEX_TCP_SOCKET_SESSION__H_)
#define __SWL_UTIL__HALF_DUPLEX_TCP_SOCKET_SESSION__H_ 1


#include "swl/util/ExportUtil.h"
#include "swl/util/GuardedBuffer.h"
#include <boost/asio.hpp>


namespace swl {

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  half duplex mode�� �̿��Ͽ� TCP socket ����� �����ϴ� server session class.
 *
 *	���������� asynchronous I/O�� ����ϰ� �����Ƿ� �� class�� ����ϴ� S/W�� �δ��� ���� �ִ� ������ �ִ�.
 *
 *	half duplex mode�� ����� �����ϹǷ� �ۼ����� �ݺ������� ����Ǿ�� �Ѵ�.
 *	���� �Ʒ��� ���� send() & receive() �Լ��� ������ ȣ��Ǿ�� �Ѵ�.
 *		- case 1
 *			-# send()
 *			-# receive()
 *			-# send()
 *			-# ...
 *		- case 2
 *			-# receive()
 *			-# send()
 *			-# receive()
 *			-# ...
 */
class SWL_UTIL_API HalfDuplexTcpSocketSession
{
public:
	//typedef HalfDuplexTcpSocketSession base_type;

public:
	/**
	 *	@brief  [ctor] contructor.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket ��� session�� ���� �ʿ��� �������� �ʱ�ȭ�Ѵ�.
	 */
	HalfDuplexTcpSocketSession(boost::asio::ip::tcp::socket &socket);
	/**
	 *	@brief  [dtor] virtual default destructor.
	 *
	 *	TCP socket ��� session�� �����ϱ� ���� �ʿ��� ������ �����Ѵ�.
	 */
	virtual ~HalfDuplexTcpSocketSession();

public:
	/**
	 *	@brief  TCP socket session�� message ���� �غ� ���¸� Ȯ��.
	 *	@return  TCP socket session�� ���� ���� ���¶�� true ��ȯ.
	 *
	 *	TCP socket session�� ���� ���� ���¿� �ִٸ� true��, �׷��� �ʴٸ� false�� ��ȯ�Ѵ�.
	 */
	bool isReadyToSend() const
	{  return state_ == SENDING;  }

	/**
	 *	@brief  TCP socket ��� �߿� �߻��� �۽� ������ error code�� ��ȯ.
	 *	@param[out]  ec  �߻��� ������ error code ��ü.
	 *
	 *	TCP socket ����� ���� message�� �����ϴ� ���� �߻��� ������ error code�� ��ȯ�Ѵ�.
	 */
	virtual void send(boost::system::error_code &ec);

	/**
	 *	@brief  TCP socket session�� message ���� �غ� ���¸� Ȯ��.
	 *	@return  TCP socket session�� ���� ���� ���¶�� true ��ȯ.
	 *
	 *	TCP socket session�� ���� ���� ���¿� �ִٸ� true��, �׷��� �ʴٸ� false�� ��ȯ�Ѵ�.
	 */
	bool isReadyToReceive() const
	{  return state_ == RECEIVING;  }

	/**
	 *	@brief  TCP socket ��� �߿� �߻��� ���� ������ error code�� ��ȯ.
	 *	@param[out]  ec  �߻��� ������ error code ��ü.
	 *
	 *	TCP socket ����� ���� message�� �����ϴ� ���� �߻��� ������ error code�� ��ȯ�Ѵ�.
	 */
	virtual void receive(boost::system::error_code &ec);

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

private:
	static const std::size_t MAX_SEND_LENGTH_ = 512;
	static const std::size_t MAX_RECEIVE_LENGTH_ = 512;

	boost::asio::ip::tcp::socket &socket_;
	enum { SENDING, RECEIVING } state_;

	GuardedByteBuffer sendBuffer_;
	GuardedByteBuffer receiveBuffer_;
	GuardedByteBuffer::value_type sendMsg_[MAX_SEND_LENGTH_];
	GuardedByteBuffer::value_type receiveMsg_[MAX_RECEIVE_LENGTH_];
	std::size_t sentMsgLength_;
};

}  // namespace swl


#endif  // __SWL_UTIL__HALF_DUPLEX_TCP_SOCKET_SESSION__H_

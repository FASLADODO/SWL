#if !defined(__SWL_TCP_SOCKET_SERVER_TEST__ECHO_TCP_SOCKET_CONNECTION__H_)
#define __SWL_TCP_SOCKET_SERVER_TEST__ECHO_TCP_SOCKET_CONNECTION__H_ 1


#include "swl/util/TcpSocketConnection.h"


namespace swl {

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  asynchronous I/O mode�� �����ϴ� TCP echo socket server�� channel ������ ó���ϴ� connection utility class.
 *
 *	server���� client�� ���� ��û�� accept�� ��, TCP socket connection ��ü�� ���ӵ� client�� ���� ���� �� ó���� �����ϰ� �Ѵ�.
 *	�������� ������ �Ʒ��� ����.
 *		-# [client] ���� ��û
 *		-# [server] client�κ��� ���� ��û ����
 *		-# [server] connection ��ü�� client�� ����
 *		-# [connection] socket ����� �̿��� client�� message �ۼ��� ����
 *		-# [server] �ٸ� client�� ���� ��û ���
 *
 *	TCP socket ����� asynchronous I/O�� �̿��Ͽ� �����Ѵ�.
 */
class EchoTcpSocketConnection: public TcpSocketConnection
{
public:
	typedef TcpSocketConnection base_type;
	typedef boost::shared_ptr<EchoTcpSocketConnection> pointer;

private:
	/**
	 *	@brief  [ctor] private constructor.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket connection ��ü�� �ʱ�ȭ�� �����Ѵ�.
	 */
	EchoTcpSocketConnection(boost::asio::io_service &ioService);
public:
	/**
	 *	@brief  [dtor] virtual default destructor.
	 *
	 *	TCP socket ��� connection�� �����ϱ� ���� �۾��� �����Ѵ�.
	 */
	virtual ~EchoTcpSocketConnection()  {}

public:
	/**
	 *	@brief  [ctor] TCP socket connection ��ü�� ������ ���� factory �Լ�.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket connection ��ü�� instance�� �����Ѵ�.
	 */
	static pointer create(boost::asio::io_service &ioService);

private:
	/**
	 *	@brief  �۽� buffer�� ����� message�� ������ ����.
	 *
	 *	�۽� buffer�� ����Ǿ� �ִ� message�� asynchronous I/O�� ���� �۽��Ѵ�.
	 */
	/*virtual*/ void doStartOperation();
	/**
	 *	@brief  �۽� ��û�� message�� ������ �Ϸ�� ��� ȣ��Ǵ� completion routine.
	 *	@param[in]  ec  message�� �����ϴ� �������� �߻��� ������ error code.
	 *	@throw  LogException  TCP socket�� close �������� error�� �߻�.
	 *
	 *	asynchronous I/O�� �̿��Ͽ� �۽� ��û�� message�� ������ �Ϸ�Ǿ��� �� system�� ���� ȣ��Ǵ� completion routine�̴�.
	 *	startSending() �Լ� ������ asynchronous �۽� ��û�� �ϸ鼭 �ش� �Լ��� completion routine���� ������ �־�� �Ѵ�.
	 */
	/*virtual*/ void doCompleteSending(boost::system::error_code ec);
	/**
	 *	@brief  TCP socket ��� channel�� ���� ���ŵ� message�� �ִ� ��� ȣ��Ǵ� completion routine.
	 *	@param[in]  ec  message�� �����ϴ� �������� �߻��� ������ error code.
	 *	@param[in]  bytesTransferred  ���ŵ� message�� ����.
	 *	@throw  LogException  TCP socket�� close �������� error�� �߻�.
	 *
	 *	asynchronous I/O�� ���� message�� ���ŵǴ� ��� system�� ���� ȣ��Ǵ� completion routine�̴�.
	 *	startReceiving() �Լ� ������ asynchronous ���� ��û�� �ϸ鼭 �ش� �Լ��� completion routine���� ������ �־�� �Ѵ�.
	 */
	/*virtual*/ void doCompleteReceiving(boost::system::error_code ec, std::size_t bytesTransferred);
};

}  // namespace swl


#endif  // __SWL_TCP_SOCKET_SERVER_TEST__ECHO_TCP_SOCKET_CONNECTION__H_

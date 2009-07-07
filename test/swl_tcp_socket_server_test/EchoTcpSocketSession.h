#if !defined(__SWL_TCP_SOCKET_SERVER_TEST__ECHO_TCP_SOCKET_SESSION__H_)
#define __SWL_TCP_SOCKET_SERVER_TEST__ECHO_TCP_SOCKET_SESSION__H_ 1


#include "swl/util/TcpSocketSession.h"


namespace swl {

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  TCP socket ����� �����ϴ� server session class.
 *
 *	TCP socket server�� connection ��ü (����: TcpSocketConnectionUsingSession) ���� ����ϱ� ���� ����� session class�̴�.
 */
class EchoTcpSocketSession: public TcpSocketSession
{
public:
	typedef TcpSocketSession base_type;

public:
	/**
	 *	@brief  [ctor] contructor.
	 *	@param[in]  socket  TCP socket ����� ���� Boost.ASIO�� socket ��ü.
	 *
	 *	TCP socket ��� session�� ���� �ʿ��� �������� �ʱ�ȭ�Ѵ�.
	 */
	EchoTcpSocketSession(boost::asio::ip::tcp::socket &socket);
	/**
	 *	@brief  [dtor] virtual default destructor.
	 *
	 *	TCP socket ��� session�� �����ϱ� ���� �ʿ��� ������ �����Ѵ�.
	 */
	virtual ~EchoTcpSocketSession();

public:
	/**
	 *	@brief  send buffer�� �ִ� message�� ����� TCP socket ��� channel�� ���� ����.
	 *	@param[out]  ec  message ���� �������� �߻��� ������ error code ��ü.
	 *
	 *	��û�� message�� TCP socket ����� ���� �����Ѵ�.
	 *
	 *	TCP socket ����� ���� message�� �����ϴ� ���� �߻��� ������ error code�� ���ڷ� �Ѿ�´�.
	 */
	/*virtual*/ void send(boost::system::error_code &ec);

	/**
	 *	@brief  TCP socket ��� channel�� ���� ���ŵ� message�� receive buffer�� ����.
	 *	@param[out]  ec  message ���� �������� �߻��� ������ error code ��ü.
	 *
	 *	TCP socket ����� ���� ���ŵ� message�� receive buffer�� �����Ѵ�.
	 *
	 *	TCP socket ����� ���� message�� �����ϴ� ���� �߻��� ������ error code�� ���ڷ� �Ѿ�´�.
	 */
	/*virtual*/ void receive(boost::system::error_code &ec);
};

}  // namespace swl


#endif  // __SWL_TCP_SOCKET_SERVER_TEST__ECHO_TCP_SOCKET_SESSION__H_

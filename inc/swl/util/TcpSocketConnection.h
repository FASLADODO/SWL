#if !defined(__SWL_UTIL__TCP_SOCKET_CONNECTION__H_)
#define __SWL_UTIL__TCP_SOCKET_CONNECTION__H_ 1


#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>


namespace swl {

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  full duplex mode�� �����ϴ� TCP socket server�� channel ������ ó���ϴ� connection utility class.
 *
 *	server���� client�� ���� ��û�� accept�� ��, TCP socket connection ��ü�� ���ӵ� client�� ���� ���� �� ó���� �����ϰ� �Ѵ�.
 *	�������� ������ �Ʒ��� ����.
 *		-# [client] ���� ��û
 *		-# [server] client�κ��� ���� ��û ����
 *		-# [server] connection ��ü�� client�� ����
 *		-# [connection] client���� ����� ���� sesseion ��ü�� ���� (full duplex mode)
 *		-# [session] socket ����� �̿��� client�� message �ۼ��� ����
 *		-# [server] �ٸ� client�� ���� ��û ���
 *
 *	�� �� client���� ����� data member�� Session ��ü�� ���� �����ϰ� �Ǵµ� Session ��ü�� �Ʒ��� ��Ҹ� ������ �־�� �Ѵ�.
 *		- type definition
 *			- pointer;
 *		- interface
 *			- static pointer create(boost::asio::io_service &ioService);
 *			- boost::asio::ip::tcp::socket & getSocket(); and/or const boost::asio::ip::tcp::socket & getSocket() const;
 *			- void start();
 *
 *	TCP socket ����� asynchronous I/O�� �̿��Ͽ� �����Ѵ�.
 */
template<typename Session>
class FullDuplexTcpSocketConnection: public boost::enable_shared_from_this<FullDuplexTcpSocketConnection>
{
public:
	//typedef FullDuplexTcpSocketConnection base_type;
	typedef Session session_type;
	typedef boost::shared_ptr<FullDuplexTcpSocketConnection> pointer;

private:
	/**
	 *	@brief  [ctor] private contructor.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket connection ��ü�� �ʱ�ȭ�� �����Ѵ�.
	 */
	FullDuplexTcpSocketConnection(boost::asio::io_service &ioService)
	: ioService_(ioService), socket_(ioService), session_(socket_)
	{}

public:
	/**
	 *	@brief  [ctor] TCP socket connection ��ü�� ������ ���� factory �Լ�.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket connection ��ü�� instance�� �����Ѵ�.
	 */
	static pointer create(boost::asio::io_service &ioService)
	{  return pointer(new FullDuplexTcpSocketConnection(ioService));  }

	/**
	 *	@brief  TCP socket ����� �����ϴ� socket ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��Ÿ� ����ϴ� socket ��ü.
	 *
	 *	������ TCP socket ����� �����ϰ� �Ǵ� socket ��ü�� reference�� ��ȯ�Ѵ�.
	 */
	boost::asio::ip::tcp::socket & getSocket()  {  return socket_;  }
	/**
	 *	@brief  TCP socket ����� �����ϴ� socket ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��Ÿ� ����ϴ� socket ��ü.
	 *
	 *	������ TCP socket ����� �����ϰ� �Ǵ� socket ��ü�� const reference�� ��ȯ�Ѵ�.
	 */
	const boost::asio::ip::tcp::socket & getSocket() const  {  return socket_;  }

	/**
	 *	@brief  TCP socket ��� �������� ���� task�� �����ϴ� session ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��� ������ task�� �����ϴ� session ��ü.
	 *
	 *	TCP socket ��� �������� message �۽� �� ������ �������� task�� �����ϴ� session ��ü�� reference�� ��ȯ�Ѵ�.
	 */
	session_type & getSession()  {  return session_;  }
	/**
	 *	@brief  TCP socket ��� �������� ���� task�� �����ϴ� session ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��� ������ task�� �����ϴ� session ��ü.
	 *
	 *	TCP socket ��� �������� message �۽� �� ������ �������� task�� �����ϴ� session ��ü�� const reference�� ��ȯ�Ѵ�.
	 */
	const session_type & getSession() const  {  return session_;  }

	/**
	 *	@brief  client�� TCP socket ����� ����.
	 *
	 *	TCP socket server�� ���� client���� ������ �̷���� �� client�� message�� �ۼ����� �����Ѵ�.
	 */
	void start()
	{
		startReceiving();
		// TODO [check] >>
		startSending();
	}

private:
	void startSending()
	{
		session_.startSending();
	}

	void startReceiving()
	{
		session_.startReceiving();
	}

private:
	boost::asio::io_service &ioService_;
	boost::asio::ip::tcp::socket socket_;

	session_type session_;
};

//-----------------------------------------------------------------------------------
//

/**
 *	@brief  half duplex mode�� �����ϴ� TCP socket server�� channel ������ ó���ϴ� connection utility class.
 *
 *	server���� client�� ���� ��û�� accept�� ��, TCP socket connection ��ü�� ���ӵ� client�� ���� ���� �� ó���� �����ϰ� �Ѵ�.
 *	�������� ������ �Ʒ��� ����.
 *		-# [client] ���� ��û
 *		-# [server] client�κ��� ���� ��û ����
 *		-# [server] connection ��ü�� client�� ����
 *		-# [connection] client���� ����� ���� sesseion ��ü�� ����
 *		-# [session] socket ����� �̿��� client�� message �ۼ��� ���� (half duplex mode)
 *		-# [server] �ٸ� client�� ���� ��û ���
 *
 *	�� �� client���� ����� data member�� Session ��ü�� ���� �����ϰ� �Ǵµ� Session ��ü�� �Ʒ��� ��Ҹ� ������ �־�� �Ѵ�.
 *		- type definition
 *			- pointer;
 *		- interface
 *			- static pointer create(boost::asio::io_service &ioService);
 *			- boost::asio::ip::tcp::socket & getSocket(); and/or const boost::asio::ip::tcp::socket & getSocket() const;
 *			- void start();
 *
 *	TCP socket ����� asynchronous I/O�� �̿��Ͽ� �����Ѵ�.
 */
template<typename Session>
class HalfDuplexTcpSocketConnection: public boost::enable_shared_from_this<HalfDuplexTcpSocketConnection>
{
public:
	//typedef HalfDuplexTcpSocketConnection base_type;
	typedef Session session_type;
	typedef boost::shared_ptr<HalfDuplexTcpSocketConnection> pointer;

private:
	/**
	 *	@brief  [ctor] private contructor.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket connection ��ü�� �ʱ�ȭ�� �����Ѵ�.
	 */
		HalfDuplexTcpSocketConnection(boost::asio::io_service &ioService)
	: ioService_(ioService), socket_(ioService), session_(socket_),
	  isReceiving_(false), isSending_(false)
	{}

public:
	/**
	 *	@brief  [ctor] TCP socket connection ��ü�� ������ ���� factory �Լ�.
	 *	@param[in]  ioService  TCP socket ����� ���� Boost.ASIO�� I/O service ��ü.
	 *
	 *	TCP socket connection ��ü�� instance�� �����Ѵ�.
	 */
	static pointer create(boost::asio::io_service &ioService)
	{  return pointer(new HalfDuplexTcpSocketConnection(ioService));  }

	/**
	 *	@brief  TCP socket ����� �����ϴ� socket ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��Ÿ� ����ϴ� socket ��ü.
	 *
	 *	������ TCP socket ����� �����ϰ� �Ǵ� socket ��ü�� reference�� ��ȯ�Ѵ�.
	 */
	boost::asio::ip::tcp::socket & getSocket()  {  return socket_;  }
	/**
	 *	@brief  TCP socket ����� �����ϴ� socket ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��Ÿ� ����ϴ� socket ��ü.
	 *
	 *	������ TCP socket ����� �����ϰ� �Ǵ� socket ��ü�� const reference�� ��ȯ�Ѵ�.
	 */
	const boost::asio::ip::tcp::socket & getSocket() const  {  return socket_;  }

	/**
	 *	@brief  TCP socket ��� �������� ���� task�� �����ϴ� session ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��� ������ task�� �����ϴ� session ��ü.
	 *
	 *	TCP socket ��� �������� message �۽� �� ������ �������� task�� �����ϴ� session ��ü�� reference�� ��ȯ�Ѵ�.
	 */
	session_type & getSession()  {  return session_;  }
	/**
	 *	@brief  TCP socket ��� �������� ���� task�� �����ϴ� session ��ü�� ��ȯ.
	 *	@return  ���� TCP socket ��� ������ task�� �����ϴ� session ��ü.
	 *
	 *	TCP socket ��� �������� message �۽� �� ������ �������� task�� �����ϴ� session ��ü�� const reference�� ��ȯ�Ѵ�.
	 */
	const session_type & getSession() const  {  return session_;  }

	/**
	 *	@brief  client�� TCP socket ����� ����.
	 *
	 *	TCP socket server�� ���� client���� ������ �̷���� �� client�� message�� �ۼ����� �����Ѵ�.
	 */
	void start()
	{
		// put the socket into non-blocking mode.
		boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
		socket_.io_control(non_blocking_io);

		doStartOperation();
	}

private:
	void doStartOperation()
	{
		// start a read operation if the third party library wants one.
		if (session_.isReadyToReceive() && !isReceiving_)
		{
			isReceiving_ = true;
			socket_.async_read_some(
				boost::asio::null_buffers(),
				boost::bind(&HalfDuplexTcpSocketConnection::completeReceiving, shared_from_this(), boost::asio::placeholders::error)
			);
		}

		// start a write operation if the third party library wants one.
		if (session_.isReadyToSend() && !isSending_)
		{
			isSending_ = true;
			socket_.async_write_some(
				boost::asio::null_buffers(),
				boost::bind(&HalfDuplexTcpSocketConnection::completeSending, shared_from_this(), boost::asio::placeholders::error)
			);
		}
	}

	void completeReceiving(boost::system::error_code ec)
	{
		isReceiving_ = false;

		// notify third party library that it can perform a read.
		if (!ec)
			session_.receive(ec);

		// the third party library successfully performed a read on the socket.
		// start new read or write operations based on what it now wants.
		if (!ec || boost::asio::error::would_block == ec)
			doStartOperation();
		// otherwise, an error occurred. Closing the socket cancels any outstanding asynchronous read or write operations.
		// the FullDuplexTcpSocketConnection object will be destroyed automatically once those outstanding operations complete.
		else
			socket_.close();
	}

	void completeSending(boost::system::error_code ec)
	{
		isSending_ = false;

		// notify third party library that it can perform a write.
		if (!ec)
			session_.send(ec);

		// the third party library successfully performed a write on the socket.
		// start new read or write operations based on what it now wants.
		if (!ec || boost::asio::error::would_block == ec)
			doStartOperation();
		// otherwise, an error occurred. Closing the socket cancels any outstanding asynchronous read or write operations.
		// the HalfDuplexTcpSocketConnection object will be destroyed automatically once those outstanding operations complete.
		else
			socket_.close();
	}

private:
	boost::asio::io_service &ioService_;
	boost::asio::ip::tcp::socket socket_;

	session_type session_;

	bool isReceiving_;
	bool isSending_;
};

}  // namespace swl


#endif  // __SWL_UTIL__TCP_SOCKET_CONNECTION__H_

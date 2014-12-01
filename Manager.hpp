#ifndef MANAGER_HPP
#define MANAGER_HPP

// Include first, because contains basic.hpp with OS ver, etc
#include "Protocol.hpp"

// Boost includes
#include <boost/asio.hpp>

// STL includes
#include <queue>

using boost::asio::ip::tcp;

// Class to manage overall client interaction with the AOL servers
class Manager
{
public: // Ctor&Dtor
	Manager(const std::string & uid, const std::string & pwd, const std::string & host, const std::string & port);
	~Manager();

public: // Constants
//	enum {LOGIN_ROASTED = 0, LOGIN_MD5, LOGIN_SSL};
	 enum {SOCKET_BUFFER_SMALL = 256, SOCKET_BUFFER_MEDIUM = 512, SOCKET_BUFFER_LARGE = 1024};
// public: // Variables
public: // Methods
	// Login to auth server using roasted password
	int LoginToAuthServer(vector & bos, vector & cookie);

	// Login to auth server using MD5 hashed password
//	int LoginMD5();

	// Login to bos server using cookie from auth server
	int LoginToBOS(const vector & bos, const vector & cookie);

private: // Variables
	boost::asio::io_service * mIOService;
	tcp::resolver * mResolver;
	tcp::resolver::query * mQuery;
	tcp::socket * mSocket;

	word mSeqNumber; // Sequence number to supply with each socket send
	std::string mUid; // ICQ User id
	std::string mPwd; // ICQ User password
	std::string mHost; // Host currently connected to
	std::string mPort; // Port currently used

	// Class representing a queue of FLAP packets that are being sent to or
	// have been received from the server
	class ioflaps
	{
	public:
		ioflaps() {}
		~ioflaps() { this->clear(); } // Release flap objects upon exit
	
	public:
		std::size_t length() const { return flaps.size(); } // Get total FLAPs count
		void pop() { return flaps.pop(); } // Remove first FLAP from the queue

		// Add new FLAP to the end of the queue
		int push()
		{
			FLAP * flap = 0;
			try
			{
				flap = new FLAP;
			}
			catch (std::bad_alloc & e)
			{
				std::cout << e.what() << std::endl;
				return 1;
			}

			flaps.push(flap);

			return 0;
		};
		FLAP * front() { return flaps.front(); } // // Get first FLAP in the queue
		FLAP * back() { return flaps.back(); } // Get last FLAP in the queue
		bool empty() { return flaps.empty(); }
		void clear()
		{
			while(!flaps.empty())
			{
				FLAP * flap = flaps.front();

				if (flap)
					delete flap;
				flap = 0;

				flaps.pop();
			}
		}
	
	private:
		std::queue<FLAP *> flaps;

	private:
		ioflaps & operator=(const ioflaps &); // Assignment not defined
		ioflaps(const ioflaps &); // Copying not defined
	};
	ioflaps iflap; // Queue of incoming flaps
	ioflaps oflap; // Queue of outgoing flaps

private: // Methods
	// Parsing routines
	//
	// Parse raw socket data and construct well-formed FLAP object
	std::size_t RawToFLAP(const vector & data, FLAP & flap);
	
	// Convert FLAP object into raw data ready to send on the socket
	int FLAPToRaw(const FLAP & flap, vector & data);

	// Produce roasted password to send within an insecure connection.
	// Hint: Supply roasted pwd as 1st param here and get origin password =)
	int RoastPassword(const std::string & pwd, vector & data);

	// Sending routines
	//
	// Pops FLAP packets from the queue, parses them and sends raw socket data to the server.
	// This method ensures that queue is empty after it completes
	int SubmitQueue();

	// Reads raw data from the server and populates FLAPs queue
	// This method ensures that queue is empty before populating it. But try
	// not to abuse with this, always clear queue when you're done with it
	int FetchQueue();

	// Initial connect to the AOL servers initiated by the client
	// to start conversation
	int InitialConnect(const std::string & host, const std::string & port);
	
	//
	Manager(const Manager &); // Copying not allowed
	Manager operator=(const Manager &); // Assignment not allowed
};

#endif // #ifndef MANAGER_HPP

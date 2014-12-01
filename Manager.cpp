#include "Manager.hpp"

#include <cstdlib> // For rand()
#include <ctime> // For time()

extern void log(const char * s);

Manager::Manager(const std::string & uid, const std::string & pwd,
				 const std::string & host, const std::string & port)
: mIOService(0)
, mResolver(0)
, mQuery(0)
, mSocket(0)
, mSeqNumber(0)
, mUid("")
, mPwd("")
, mHost("")
, mPort("")
{
	try
	{
		mIOService = new boost::asio::io_service;
		mResolver = new tcp::resolver(*mIOService);
		mSocket = new tcp::socket(*mIOService);
	}
	catch (std::bad_alloc & exc)
	{
		// TODO: Differentiate which has allocated, which not, or will leak!
		std::cout << exc.what() << std::endl;
		return;
	}

	// Init random seq number, beware here, not any seqnum seems to be accepted
	// TODO: Consider replacing with Boost rand
	srand((unsigned)time(0));
	mSeqNumber = (word)rand(); // 0 - 32767

	mUid = uid;
	mPwd = pwd;
	mHost = host;
	mPort = port;
}

Manager::~Manager()
{
//	TODO: close socket here

	if (mSocket)
		delete mSocket;
	mSocket = 0;

	if (mResolver)
		delete mResolver;
	mResolver = 0;

	if (mIOService)
		delete mIOService;
	mIOService = 0;
}

// Returns number of processed bytes, this helps us to identify if we have
// several FLAPs within one raw data
std::size_t Manager::RawToFLAP(const vector & data, FLAP & flap)
{
	// TODO: Returning 1 is bad as we're returning number of bytes. Switch to
	// exceptions?
	if (data.length() < FLAP::kHeaderLength || &flap == 0)
		return 1;

	// Reading FLAP header - always kHeaderLength bytes length!
	flap.packetsign() = data[0]; // Always 0x2a (must test this)
	flap.channel() = data[1];

	// Reading Sequence Number
	// Read second byte first, since x86 is little-endian
	byte b[] = {data[3], data[2]};

	// If this ever works dirty, consider replacing with union {word n; byte b[2];}
	flap.seqnumber() = *(reinterpret_cast<word *>(b)); // Cast away char * to short *

	// Reading Data Field Length
	b[0] = data[5];
	b[1] = data[4];
	flap.length() = *(reinterpret_cast<word *>(b));

	// Reading FLAP body
	if (flap.channel() == FLAP::CHANNEL_NEW_CONNECTION)
	{
		// For channel 0x01 the data is actually 4-bytes Protocol Version
		byte b4[] = {data[FLAP::kHeaderLength + 3], data[FLAP::kHeaderLength + 2], data[FLAP::kHeaderLength + 1], data[FLAP::kHeaderLength]};
		flap.protocol() = *(reinterpret_cast<dword *>(b4));
	}
	else if (flap.channel() == FLAP::CHANNEL_FNAC_DATA)
	{
		// Note: FNACs are never transmitted on any channel other than 0x02
		// Note: There can be only one FNAC per FLAP command
		flap.AddFNAC();

		b[0] = data[FLAP::kHeaderLength + 1];
		b[1] = data[FLAP::kHeaderLength];
		flap.fnac().family() = *(reinterpret_cast<word *>(b));

		b[0] = data[FLAP::kHeaderLength + 3];
		b[1] = data[FLAP::kHeaderLength + 2];
		flap.fnac().subtype() = *(reinterpret_cast<word *>(b));

		b[0] = data[FLAP::kHeaderLength + 5];
		b[1] = data[FLAP::kHeaderLength + 4];
		flap.fnac().flags() = *(reinterpret_cast<word *>(b));

		byte b4[4] = {data[FLAP::kHeaderLength + 9], data[FLAP::kHeaderLength + 8], data[FLAP::kHeaderLength + 7], data[FLAP::kHeaderLength + 6]};
		flap.fnac().id() = *(reinterpret_cast<dword *>(b4));
	}
	else if (flap.channel() == FLAP::CHANNEL_CLOSE_CONNECTION)
	{
		// For channel 0x04 the data is actually set of TLVs (4 ones)
		// The idea is to read TLV type first (word),
		// then length (word) and then data of <len> size, and yet again
		// from the start till we reach the body end

		// Reading FLAP body, length = flap.mDataFieldLength
		vector body;
		body.assign(&data[FLAP::kHeaderLength], &data[FLAP::kHeaderLength + flap.length() - 1]);
//		assert(flap.mDataFieldLength == body)

		std::size_t offset = 0;
		byte type[2], len[2];
		while (offset < body.length())
		{
			// TLV data type
			type[0] = body[offset + 1];
			type[1] = body[offset + 0];

			// TLV data length
			len[0] = body[offset + 3];
			len[1] = body[offset + 2];
			
			// TLV data
			vector data;
			data.assign(&body[offset + FLAP::TLV::kHeaderLength], *(reinterpret_cast<word *>(len)));
			flap.AddTLV(*(reinterpret_cast<word *>(type)), data);
			
			offset += FLAP::TLV::kHeaderLength + data.length();
		}
	}
	
	return FLAP::kHeaderLength + flap.length();
}

int Manager::FLAPToRaw(const FLAP & flap, vector & data)
{
	if (&flap == 0)
		return 1;

	// Filling FLAP header - always FLAP::kHeaderLength bytes len!
	// Data len is calculated at the method end
	data << flap.packetsign() << flap.channel() << flap.seqnumber() << (word)0x0000;

	// Filling FLAP body
	if (flap.channel() == FLAP::CHANNEL_NEW_CONNECTION)
	{
		data << flap.protocol();

		// TODO: Check that needed TLV exists
		for (int i = 0; i < flap.GetTLVCount(); i++)
			data << flap.mTLVArray[i]->type() << flap.mTLVArray[i]->len() << flap.mTLVArray[i]->val();

		// Calculate FLAP body length: all data minus FLAP header length
		union {byte b[2]; word val;} u;
		u.val = (word)data.length() - FLAP::kHeaderLength;
		data[5] = u.b[0];
		data[4] = u.b[1];
	}
	else if (flap.channel() == FLAP::CHANNEL_FNAC_DATA)
	{
		// Note: FNACs are never transmitted on any channel other than 0x02
		// Note: There can be only one FNAC per FLAP command 
		data << flap.fnac().family();
		data << flap.fnac().subtype();
		data << flap.fnac().flags();
		data << flap.fnac().id();

		// The mData length = FLAP mDataFieldLength minus FNAC header length
		// TODO: FNAC data appears to be FLAP data. Work this out!
//		data << flap.fnac().data();
	}

	return 0;
}

int Manager::RoastPassword(const std::string & pwd, vector & data)
{
	const byte roast[] = {0xf3, 0x26, 0x81, 0xc4, 0x39, 0x86, 0xdb, 0x92, 0x71, 0xa3, 0xb9, 0xe6, 0x53, 0x7a, 0x95, 0x7c};

	for (std::size_t i = 0; i < pwd.length(); i++)
	{
		data << (byte)(pwd[i] ^ roast[i]);
	}

	return 0;
}

int Manager::SubmitQueue()
{
//	assert(mSocket)

	// Iterate through all FLAPs in the outgoing queue and construct raw data to be sent
	vector data, buf;
	while (!oflap.empty())
	{
		// Obtain pending FLAP and construct raw data from it
		FLAPToRaw(*oflap.front(), buf);
		data << buf;
		
		// Clear buffer for further reads
		buf.clear();

		// Remove processed FLAP from the queue
		oflap.pop();
	}

	// We now have complete data to be sent, let's do it
	try
	{
		boost::system::error_code error;
		mSocket->write_some(boost::asio::buffer(data.data(), data.length()), error);
		if (error)
			throw boost::system::system_error(error);
	}
	catch (std::exception & exc)
	{
		std::cerr << exc.what() << std::endl;
		return 1;
	}

	return 0;
}

int Manager::FetchQueue()
{
	// Make sure we are working with initially empty queue
	if (!iflap.empty())
		iflap.clear();

	// TODO: Make sure we read complete data from the socket
	vector data;
	byte buf[SOCKET_BUFFER_MEDIUM];

	// TODO: Test this on the data > SOCKET_BUFFER_MEDIUM !
	std::size_t len = 0;
	for (;;)
	{
		// Read socket data, receive() routine is guaranteed to read exact size,
		// or less if no more data left
		// TODO: Check for errors
//		boost::system::error_code error;
//		std::size_t len = 0;
		try
		{
			len = mSocket->receive(boost::asio::buffer(buf, SOCKET_BUFFER_MEDIUM));
		}
		catch (std::exception & exc)
		{
			std::cerr << exc.what() << std::endl;
			return 1;
		}
		
		data.append(buf, len);

		// When the server closes the connection, read_some() will return
		// the boost::asio::error::eof error, which is how we know to exit the loop.
		// TODO: If this doesn't work, consider comparing len with SOCKET_BUFFER_MEDIUM
//		if (error == boost::asio::error::eof)
//			break; // Connection closed cleanly by peer.
		if (len < SOCKET_BUFFER_MEDIUM)
			break;
//		else if (error)
//			throw boost::system::system_error(error); // Some other error
	}

	// We now have complete data the server sent to us, let's parse it.
	// Note that sending several FLAPs within one socket send appears to be
	// a normal practice, and so we must respect this and always produce correct
	// number of FLAPS.
	// TODO: Revise the code below, perhaps needs smarter implementation
	len = 0;
	vector flapdata;
	while (len < data.length())
	{
		iflap.push();
		flapdata.assign(&data[len], data.length() - len);
		len += RawToFLAP(flapdata, *iflap.back());
	}
	
	return 0;
}

int Manager::InitialConnect(const std::string & host, const std::string & port)
{
	try
	{
		tcp::resolver::query query(host, port);

		tcp::resolver::iterator endpoint_iterator = mResolver->resolve(query);
		tcp::resolver::iterator end;
		boost::system::error_code error = boost::asio::error::host_not_found;

		while (error && endpoint_iterator != end)
		{
			mSocket->close();
			mSocket->connect(*endpoint_iterator++, error);
		}

		if (error)
			throw boost::system::system_error(error);
	}
	catch (std::exception & exc)
	{
		std::cerr << exc.what() << std::endl;
		return 1;
	}

	return 0;
}

// http://www.oilcan.org/oscar/section3.html
int Manager::LoginToAuthServer(vector & bos, vector & cookie)
{
//	assert (mHost, mPort not empty)
	try
	{
		// Connect to the authorization server
		InitialConnect(mHost, mPort);

		// Read "Connection Acknowledge" response from the server, 10 bytes.
		// This is sent by the server after a new connection has been opened
		// and is ready for duplex operation
		FetchQueue();

		// If we don't use fetched data, clear the incoming queue before further use
		iflap.clear();
		
		// Send "Authorization Request"
		oflap.push();
		oflap.front()->channel() = FLAP::CHANNEL_NEW_CONNECTION;
		oflap.front()->seqnumber() = mSeqNumber++;
//		oflap.front()->length(); // This will be set by the parsing routine
		oflap.front()->protocol() = 1;

		oflap.front()->AddTLV(FLAP::TLV::TYPE_SCREEN_NAME, mUid);
		vector roasted;
		RoastPassword(mPwd, roasted);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_PASSWORD_ROASTED, roasted);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_ID_STRING, "ICQ Client");
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_MAJOR_VERSION, (word)6);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_MINOR_VERSION, (word)5);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_LESSER_VERSION, (word)0);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_BUILD_NUMBER, (word)2024);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_ID_NUMBER, (word)266);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_DISTRIBUTION_NUMBER, (dword)30007);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_LANGUAGE, "en");
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_COUNTRY, "us");
		oflap.front()->AddTLV(FLAP::TLV::TYPE_UNKNOWN_3, (byte)0);

		// Send flap to the server
		SubmitQueue();
		
		// Read "Authorization Response" from the server and parse it
		FetchQueue();

		// Check authorization errors.
		// TODO: Here we just check for presence of tlv with type TYPE_ERROR_CODE,
		// we probably need to check val of that tlv as well, to work out
		// the exact reason of login failure. For now in case of error we will treat
		// it as login/password user mistyping
		for (int i = 0; i < iflap.front()->GetTLVCount(); i++)
		{
			if (iflap.front()->mTLVArray[i]->type() == FLAP::TLV::TYPE_ERROR_CODE)
				return 1; // Tell user to re-enter login/password
		}

		// The "Authorization Response" was successful, obtain bos and cookie
		for (int i = 0; i < iflap.front()->GetTLVCount(); i++)
		{
			if (iflap.front()->mTLVArray[i]->type() == FLAP::TLV::TYPE_SERVER_BOS_STRING)
				bos = iflap.front()->mTLVArray[i]->val();
			else if (iflap.front()->mTLVArray[i]->type() == FLAP::TLV::TYPE_AUTHORIZATION_COOKIE)
				cookie = iflap.front()->mTLVArray[i]->val();
		}
		iflap.clear(); // Clear the queue once we're done with it
		
		// All OK, send connection close and carry on with logging in to bos
		oflap.push();
		oflap.front()->channel() = FLAP::CHANNEL_CLOSE_CONNECTION;
		oflap.front()->seqnumber() = mSeqNumber++;
		oflap.front()->length() = 0;
		SubmitQueue();

		LoginToBOS(bos, cookie);
	}
	catch (std::exception & exc)
	{
		std::cerr << exc.what() << std::endl;
		return 1;
	}

	return 0;
}

int Manager::LoginToBOS(const vector & bos, const vector & cookie)
{
//	assert (bos.length() && cookie.length());

	try
	{
		// Split bos, which looks like ip:port into ip and port
		mHost.assign((char *)bos.data(), bos.length());
		std::size_t pos = mHost.find(':');
		mPort = mHost.substr(pos + 1, mHost.length());
		mHost.resize(pos);

		InitialConnect(mHost, mPort);

		FetchQueue();
		// Test the len
//		assert(iflap.front() == 10);
		iflap.clear();

		// Send one flap to the bos, containing a cookie (along with other info)
		// and read back a list of supported services
		oflap.push();
		oflap.front()->channel() = FLAP::CHANNEL_NEW_CONNECTION;
		oflap.front()->seqnumber() = mSeqNumber++;
		oflap.front()->protocol() = 1;
		oflap.front()->AddTLV(FLAP::TLV::TYPE_AUTHORIZATION_COOKIE, cookie);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_ID_STRING, "ICQ Client");
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_MAJOR_VERSION, (word)6);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_MINOR_VERSION, (word)5);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_LESSER_VERSION, (word)0);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_BUILD_NUMBER, (word)2024);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_ID_NUMBER, (word)266);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_DISTRIBUTION_NUMBER, (dword)30007);
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_LANGUAGE, "en");
		oflap.front()->AddTLV(FLAP::TLV::TYPE_CLIENT_COUNTRY, "us");
		oflap.front()->AddTLV(FLAP::TLV::TYPE_UNKNOWN_3, (byte)0); // TODO: What does this TLV mean?
		oflap.front()->AddTLV(FLAP::TLV::TYPE_UNKNOWN_2, (dword)0x00100000); // TODO: What does this TLV mean?

		SubmitQueue();
		
		// Read a list of supported services
		FetchQueue();
		iflap.clear();
	}
	catch (std::exception & exc)
	{
		std::cerr << exc.what() << std::endl;
		return 1;
	}

	return 0;
}

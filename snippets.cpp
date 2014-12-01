2a 01 02 8f 00 0c 00 00 00 01 80 03 00 04 00 10 00 00



// Init Neon
	ne_sock_init();

	ne_socket * sock;
	sock = ne_sock_create();

	ne_sock_connect_timeout(sock, 1);
	ne_sock_read_timeout(sock, 1);

//	205.188.251.43 - login.icq.com - auth server
//	10.120.10.254 - buddyart-d04a-sr1.blue.aol.com - bos server

	ne_inet_addr * ia;
	ia = ne_iaddr_parse("205.188.251.43", ne_iaddr_ipv4);

	//
	// LOGIN STAGE START
	//
	// Initial connect to login server
	int res = ne_sock_connect(sock, ia, 5190);

	// Read Connection Acknowledge (10 bytes)
	char buf[512];
	memset(buf, 0, 512);
	ne_sock_peek(sock, buf, 512); // Read socket
	ne_sock_fullread(sock, buf, 10);

	// Send New Connection
	Manager mgr;
	word seqNumber = 32569; // Beware here, not any seqnum seems to be accepted
	
	FLAP flapConnect;
	flapConnect.mChannelId = FLAP::CHANNEL_NEW_CONNECTION;
	flapConnect.mSeqNumber = seqNumber;
	flapConnect.mDataFieldLength = 12;
	flapConnect.mProtocolVersion = 1;

	FLAP::TLV tlv;
	tlv.mType = FLAP::TLV::TYPE_UNKNOWN_2;
	tlv.mLength = 4;
	tlv.mData << (byte)0x00 << (byte)0x10 << (byte)0x00 << (byte)0x00;
	flapConnect.AddTLV(tlv);

	vector vec;
	mgr.FLAPToRaw(flapConnect, vec);

	std::cout << "vec len: " << vec.length() << std::endl; // Must be 18dec

	ne_iovec * data;
	data = new ne_iovec[vec.length()];
	for (std::size_t i = 0; i < vec.length(); i++)
	{
		data[i].base = new byte(0);
		data[i].len = 1;
	}

	for (std::size_t i = 0; i < vec.length(); i++)
	{
		memcpy(data[i].base, &vec[i], 1);
	}

//	ne_sock_fullwritev(sock, data, vec.length());
	ne_sock_fullwrite(sock, (const char *)vec.data(), vec.length());

//	ne_sock_peek(sock, buf, 512); // Read socket

	for (std::size_t i = 0; i < vec.length(); i++)
		delete data[i].base;

	delete [] data;




// Send Sign-On
FLAP flapSignOn;
flapSignOn.mChannelId = FLAP::CHANNEL_FNAC_DATA;
flapSignOn.mSeqNumber = ++seqNumber;
flapSignOn.mDataFieldLength = 0x0017; // 23dec

FLAP::FNAC fnac;
fnac.mFamilyId = FLAP::FNAC::FAMILY_SIGNON;
fnac.mSubtypeId = FLAP::FNAC::SUBTYPE_SIGNON;
fnac.mFlags = 0;
fnac.mId = 0;

// FNAC data
// TODO: The code below is dirty hard-coding, specific data format for Sign-On ?
// Infotype
fnac.mData += (char)0;
fnac.mData += (char)1;

fnac.mData += (char)0; // This zero byte is important! Separates Infotype (00 01) from Buddy section

// Buddy
fnac.mData += 9; // UID length

char uin[] = {0x32, 0x32, 0x36, 0x34, 0x37, 0x35, 0x34, 0x35, 0x39}; // UIN
fnac.mData.append(uin, 9);
flapSignOn.AddFNAC(fnac);

s.clear();
parser.FLAPToRaw(flapSignOn, s);

ne_sock_fullwrite(sock, s.data(), s.length());

memset(buf, 0, 512);
ne_sock_peek(sock, buf, 512); // Read Sign-on reply


// Send Log-On
FLAP flapLogOn;
flapSignOn.mChannelId = FLAP::CHANNEL_FNAC_DATA;
flapSignOn.mSeqNumber = ++seqNumber;
flapSignOn.mDataFieldLength = 116; // Do we really need to set this here? It's kinda awkward, better to calc it when cooking raw

//	Reuse previously declared (and used) FNAC
fnac.mFamilyId = FLAP::FNAC::FAMILY_SIGNON;
fnac.mSubtypeId = FLAP::FNAC::SUBTYPE_LOGON;
fnac.mFlags = 0;
fnac.mId = 0;

// FNAC data
fnac.mData.clear();

FLAP::TLV tlvLogOn;
tlvLogOn.mType = FLAP::TLV::TYPE_SCREEN_NAME;
tlvLogOn.mLength = 9;
tlvLogOn.mData = uin;
flapConnect.AddTLV(tlv);

tlvLogOn.mType = FLAP::TLV::TYPE_PASSWORD_MD5_HASH;
tlvLogOn.mLength = 16;
//	char md5[] = {0x82, 0x5a, 0xdc, 0x08, 0xc6, 0x0e, 0x8d, 0xd6, 0x72, 0xb1, 0xc1, 0x52, 0xac, 0xa3, 0xc9, 0x47};
char md5[] = {0x32, 0x32, 0x36, 0x34, 0x37, 0x35, 0x34, 0x35, 0x39};
tlvLogOn.mData = md5;
flapConnect.AddTLV(tlv);

tlvLogOn.mType = FLAP::TLV::TYPE_UNKNOWN_1;
tlvLogOn.mLength = 0;
tlvLogOn.mData = "";

tlvLogOn.mType = FLAP::TLV::TYPE_CLIENT_ID_STRING;
tlvLogOn.mLength = 10;
tlvLogOn.mData = "ICQ Client";

tlvLogOn.mType = FLAP::TLV::TYPE_CLIENT_MAJOR_VERSION;
tlvLogOn.mLength = 2;
char ver[] =  {0x00, 0x06};
tlvLogOn.mData.assign(ver, 2);

tlvLogOn.mType = FLAP::TLV::TYPE_CLIENT_MINOR_VERSION;
tlvLogOn.mLength = 2;
ver[0] = 0x00;
ver[1] = 0x05;
tlvLogOn.mData.assign(ver, 2);

tlvLogOn.mType = FLAP::TLV::TYPE_CLIENT_LESSER_VERSION;
tlvLogOn.mLength = 2;
ver[0] = 0x00;
ver[1] = 0x00;
tlvLogOn.mData.assign(ver, 2);

tlvLogOn.mType = FLAP::TLV::TYPE_CLIENT_BUILD_NUMBER;
tlvLogOn.mLength = 2;
ver[0] = 0x07;
ver[1] = 0xe8;
tlvLogOn.mData.assign(ver, 2);

tlvLogOn.mType = FLAP::TLV::TYPE_CLIENT_ID_NUMBER;
tlvLogOn.mLength = 2;
ver[0] = 0x01;
ver[1] = 0x0a;
tlvLogOn.mData.assign(ver, 2);



//
// LOGIN STAGE END
//

ne_iaddr_free(ia);
ne_sock_close(sock);

// De-init Neon
ne_sock_exit();

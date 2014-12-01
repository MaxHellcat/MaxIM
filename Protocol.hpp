#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "basic.hpp"

// STL 
#include <iostream>
#include <vector>

// Internal
#include "vector.hpp"


// TODO: FNAC data is really of any format, could be tlv, could be ssi, etc, how to respect this?
class FLAP
{
public: // Ctor&Dtor
	FLAP();
	~FLAP();

public: // Constants and variables
	static const byte kPacketStart = (byte)0x2a;
	static const byte kHeaderLength = (byte)0x06;
	
	enum ChannelId
	{
		CHANNEL_NEW_CONNECTION = 0x01,
		CHANNEL_FNAC_DATA = 0x02,
		CHANNEL_FLAP_LEVEL_ERROR = 0x03,
		CHANNEL_CLOSE_CONNECTION = 0x04,
		CHANNEL_KEEP_ALIVE = 0x05
	};

	class FNAC
	{
	public: // Ctor&Dtor
		FNAC() : mFamilyId(0), mSubtypeId(0), mFlags(0), mId(0)/*, mData(0)*/ {}
		~FNAC() {}

	public: // Constants and variables
		static const byte kHeaderLength = (byte)0x0a;

		enum Family
		{
			FAMILY_AIM_GENERIC = 0x0001, FAMILY_AIM_SIGNON = 0x0017
		};

		enum FamilyAIMGeneric // Subtypes of FAMILY_AIM_GENERIC family
		{
			SUBTYPE_SERVER_READY = 0x0003,
			SUBTYPE_WELL_KNOWN_URL = 0x0015
		};

		enum FamilyAIMSignon // Subtypes of FAMILY_AIM_SIGNON family
		{
			SUBTYPE_TBD1 = 0x0001,
			SUBTYPE_LOGON = 0x0002,
			SUBTYPE_LOGON_REPLY = 0x0003,
			SUBTYPE_TBD2 = 0x0003,
			SUBTYPE_SIGNON = 0x0006,
			SUBTYPE_SIGNON_REPLY = 0x0007
		};
	public: // Methods
		word & family() { return mFamilyId; }
		const word & family() const { return mFamilyId; }

		word & subtype() { return mSubtypeId; }
		const word & subtype() const { return mSubtypeId; }

		word & flags() { return mFlags; }
		const word & flags() const { return mFlags; }

		dword & id() { return mId; }
		const dword & id() const { return mId; }

//		vector & data() { return mData; }
//		const vector & data() const { return mData; }

	private: // Variables
		word mFamilyId;
		word mSubtypeId;
		word mFlags;
		dword mId;
		// Obtain len from FLAP mDataFieldLength minus FNAC header len
		// TODO: FNAC data appears to be a FLAP data. Sort this out!
//		vector mData;

	private: // Methods
		FNAC(const FNAC &); // Copying not defined
		FNAC & operator=(const FNAC &); // Assignment not defined
	};

	class TLV
	{
	public: // Ctor&Dtor
		TLV() : mType(0), mLength(0), mData(0) {}
		~TLV() {}

	public: // Constants and variables
		static const byte kHeaderLength = (byte) 0x04;

		enum // Please keep sorted
		{
			TYPE_SCREEN_NAME = 0x0001,
			TYPE_PASSWORD_ROASTED = 0x002,
			TYPE_CLIENT_ID_STRING = 0x0003,
			TYPE_ERROR_URL = 0x0004,
			TYPE_SERVER_BOS_STRING = 0x0005,
			TYPE_AUTHORIZATION_COOKIE = 0x0006,
			TYPE_ERROR_CODE = 0x0008,
			TYPE_CLIENT_COUNTRY = 0x000e,
			TYPE_CLIENT_LANGUAGE = 0x000f,
			TYPE_CLIENT_DISTRIBUTION_NUMBER = 0x0014,
			TYPE_CLIENT_ID_NUMBER = 0x0016,
			TYPE_CLIENT_MAJOR_VERSION = 0x0017,
			TYPE_CLIENT_MINOR_VERSION = 0x0018,
			TYPE_CLIENT_LESSER_VERSION = 0x0019,
			TYPE_CLIENT_BUILD_NUMBER = 0x001a,
			TYPE_PASSWORD_MD5_HASH = 0x0025,
			TYPE_UNKNOWN_1 = 0x004c,
			TYPE_UNKNOWN_4 = 0x008e,
			TYPE_UNKNOWN_3 = 0x0094,
			TYPE_UNKNOWN_2 = 0x8003
		};

	public: // Methods
		word & type() { return mType; }
		word & len() { return mLength; }
		vector & val() { return mData; }

	private: // Variables
		word mType;
		word mLength;
		vector mData;

	private: // Methods
		TLV(const TLV &); // Copying not defined
		TLV & operator=(const TLV &); // Assignment not defined
	};

	std::vector<TLV *> mTLVArray;

public: // Methods
	byte & packetsign() { return mPacketStart; }
	const byte & packetsign() const { return mPacketStart; }

	byte & channel() { return mChannelId; }
	const byte & channel() const{ return mChannelId; }

	word & seqnumber() { return mSeqNumber; }
	const word & seqnumber() const { return mSeqNumber; }

	word & length() { return mDataFieldLength; }
	const word & length() const { return mDataFieldLength; }

	dword & protocol() { return mProtocolVersion; }
	const dword & protocol() const { return mProtocolVersion; }

	int AddTLV(word type, byte val, word len = 1);
	int AddTLV(word type, word val, word len = 2);
	int AddTLV(word type, dword val, word len = 4);
	int AddTLV(word type, const char * val);
	int AddTLV(word type, const std::string & val);
	int AddTLV(word type, const vector & val);
	int GetTLVCount() const { return mTLVArray.size(); }

	FNAC & fnac() { return *mFnac; }
	const FNAC & fnac() const { return *mFnac; }

	int AddFNAC();

private: // Variables
	byte mPacketStart; // The first byte of the FLAP packet, always 0x2a
	byte mChannelId; // FLAP channel id
	word mSeqNumber; // FLAP sequence number, increments within each FLAP send
	word mDataFieldLength; // Total len of FLAP data
	dword mProtocolVersion; // Always 0x00000001 and for channel 0x01 only
	
	FNAC * mFnac; // The one and only FNAC

private: // Methods
	// Allocates memory for one TLV object, passing ref is essential
	int AllocateTLVMemory(TLV * &ptlv);
};

#endif // #ifdef PROTOCOL_HPP

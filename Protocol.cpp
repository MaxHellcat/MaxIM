#include "Protocol.hpp"

FLAP::FLAP()
: mPacketStart(FLAP::kPacketStart)
, mChannelId(0)
, mSeqNumber(0)
, mDataFieldLength(0)
, mProtocolVersion(0)
, mFnac(0)
{}

FLAP::~FLAP()
{
	// Release array of TLVs
	for (std::size_t i = 0; i < mTLVArray.size(); i++)
	{
		if (mTLVArray[i])
			delete mTLVArray[i];
		mTLVArray[i] = 0;
	}

/*
	// Ditto for array of FNACS
	// It's len ever more than one?
	for (int i = 0; i < mFNACArray.size(); i++)
	{
		if (mFNACArray[i])
			delete mFNACArray[i];
		mFNACArray[i] = 0;
	}
*/
}

int FLAP::AllocateTLVMemory(TLV * &ptlv)
{
	try
	{
		ptlv = new TLV;
	}
	catch (std::bad_alloc & e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
	return 0;
}

int FLAP::AddTLV(word type, byte val, word len)
{
	TLV * ptlv = 0;
	if (AllocateTLVMemory(ptlv))
		return 1;

	ptlv->type() = type;
	ptlv->len() = len;
	ptlv->val() = val;

	mTLVArray.push_back(ptlv);

	return 0;
}

int FLAP::AddTLV(word type, word val, word len)
{
	TLV * ptlv = 0;
	if (AllocateTLVMemory(ptlv))
		return 1;

	ptlv->type() = type;
	ptlv->len() = len;
	ptlv->val() = val;

	mTLVArray.push_back(ptlv);

	return 0;
}

// TODO: Consider merging all AddTLV into one!
int FLAP::AddTLV(word type, dword val, word len)
{
	TLV * ptlv = 0;
	if (AllocateTLVMemory(ptlv))
		return 1;

	ptlv->type() = type;
	ptlv->len() = len;
	ptlv->val() = val;

	mTLVArray.push_back(ptlv);

	return 0;
}

int FLAP::AddTLV(word type, const char * val)
{
	TLV * ptlv = 0;
	if (AllocateTLVMemory(ptlv))
		return 1;

	ptlv->type() = type;
	ptlv->len() = (word)strlen(val);
	ptlv->val() = val;

	mTLVArray.push_back(ptlv);

	return 0;
}

int FLAP::AddTLV(word type, const std::string & val)
{
	TLV * ptlv = 0;
	if (AllocateTLVMemory(ptlv))
		return 1;

	ptlv->type() = type;
	ptlv->len() = (word)val.length();
	ptlv->val() = val;

	mTLVArray.push_back(ptlv);

	return 0;
}

int FLAP::AddTLV(word type, const vector & val)
{
	TLV * ptlv = 0;
	if (AllocateTLVMemory(ptlv))
		return 1;

	ptlv->type() = type;
	ptlv->len() = (word)val.length();
	ptlv->val() = val;

	mTLVArray.push_back(ptlv);

	return 0;
}

// Rework to push pointers instead
int FLAP::AddFNAC()
{
	FNAC * pfnac;
	try
	{
		pfnac = new FNAC;
	}
	catch (std::bad_alloc & e)
	{
		std::cout << e.what() << std::endl;
		pfnac = 0;
		return 1;
	}
	mFnac = pfnac;

	return 0;
}

#include "vector.hpp"

// Ctor by default allocates kBytesToReserve bytes
vector::vector(std::size_t len) : mData(0), mLength(0)
{
	// Allocate memory
	try
	{
		mData = new byte[len + kBytesToReserve];
	}
	catch (std::bad_alloc & exc)
	{
		mData = 0;
		mLength = 0;
		mAllocated = 0;
		std::cout << exc.what() << std::endl;
		return;
	}

	// Fill with zeros
//	memset(mData, 0, len + kBytesToReserve);

	// Initialize internal variables
	mAllocated = len + kBytesToReserve;
	mLength = len;
}

vector::~vector()
{
	// Release allocated memory
	if (mData)
		delete [] mData;
	mData = 0;
	mLength = 0;
	mAllocated = 0;
}

vector::vector(const vector & rhs)
{
	// Allocate memory for the destination object
	try
	{
		mData = new byte[rhs.mAllocated];
	}
	catch (std::bad_alloc & exc)
	{
		mData = 0;
		mLength = 0;
		mAllocated = 0;
		std::cout << exc.what() << std::endl;
	}

	// We only need to copy mLength of bytes
	memmove_s(mData, rhs.mLength, rhs.mData, rhs.mLength);

	// Copy variables
	mLength = rhs.mLength;
	mAllocated = rhs.mAllocated;
}

/*
void vector::log(const char * s) const
{
	s = s;
#ifdef IM_DEBUG
	std::cout << s << std::endl;
#endif
}
*/

// Return ref to element with range check
// If range failed return 1st elem 
byte & vector::operator[](std::size_t index) const
{
	if (index >= mLength)
	{
		throw std::out_of_range("Out of range");
//		return mData[0];
	}

	return mData[index];
}

vector & vector::operator<<(const byte val)
{
	union {byte s[1]; byte n;} u1;
	u1.n = val;
	return append(u1.s, u1.s);
}

vector & vector::operator<<(const word val)
{
	union {byte s[2]; word n;} u2;
	u2.n = val;
	return append(&u2.s[1], u2.s);
}

vector & vector::operator<<(const dword & val)
{
	union {byte s[4]; dword n;} u4;
	u4.n = val;
	return append(&u4.s[3], u4.s);
}

vector & vector::operator=(const byte val)
{
	union {byte s[1]; byte n;} u1;
	u1.n = val;
	return assign(u1.s, u1.s);
}

vector & vector::operator=(const word val)
{
	union {byte s[2]; word n;} u2;
	u2.n = val;
	return assign(&u2.s[1], u2.s);
}

vector & vector::operator=(const dword & val)
{
	union {byte s[4]; dword n;} u4;
	u4.n = val;
	return assign(&u4.s[3], u4.s);
}

// Copies bytes starting p1 ending p2 to the data (all inclusive).
// The memory will be reallocated if isn't big enough.
// The memory will not be truncated if bigger than array, values after array
// will be left as is.
// Can accept reversed data, i.e. p1 > p2, in this case will be reverse written.
// The length is *always* reset to the assigned buffer length
vector & vector::assign(const byte * p1, const byte * p2, size_t offset/* = 0*/)
{
	if (p1 == 0 || p2 == 0)
		return *this;

	bool reverse = false; // Whether we need to reverse data before final writing
	if (p1 > p2)
		reverse = true;

	std::size_t delta;
	if (!reverse) // TODO: Replace with abs
		delta = p2 - p1 + 1; // + 1 respects p2 value
	else
		delta = p1 - p2 + 1; // + 1 respects p1 value

	// Check if delta doesn't fit into current allocation or we need to reallocate memory
	if (offset + delta <= mAllocated)
	{
		// Good, we have a room to use.
		// No need to copy old data as we aren't reallocating
		if (!reverse)
			memmove_s(mData + offset, delta, p1, delta);
		else
		{
			int l = delta;
			while (l--) { (mData + offset)[delta - l - 1] = p2[l]; } // reversed memmove (custom)
		}

		// Adjust the length
		mLength = offset + delta;

		log("vector: Assigned without reallocation");
	}
	else
	{
		// Ouch, we've used all the reserved space, let's reallocate our memory
		byte * p;
		try
		{
			p = new byte[offset + delta + kBytesToReserve];
		}
		catch (std::bad_alloc & exc)
		{
			std::cout << exc.what() << "Unable to reallocate memory!" << std::endl;
			return *this;
		}

		// Fill with zeros new allocated data
//		memset(p, 0, offset + delta + kBytesToReserve);

		// Copy old mLength bytes to a new location, we want to keep them.
		// We don't want to copy more than mLength bytes, because that will
		// be overwritten anyway
		memmove_s(p, mLength, mData, mLength);

		// Copy source 
		if (!reverse)
			memmove_s(p + offset, delta, p1, delta);
		else
		{
			int l = delta;
			while (l--) { (p + offset)[delta - l - 1] = p2[l]; } // reversed memmove (custom)
		}

		// Release old data and replace pointers
		delete [] mData;
		mData = p;

		// Recalculate reserved memory and actual length
		mLength = offset + delta;
		mAllocated = mLength + kBytesToReserve;

		log("vector: Assigned with reallocation");
	}

	return *this;
}

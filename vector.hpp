#ifndef VECTOR_HPP
#define VECTOR_HPP

#include "basic.hpp"

//#define IM_DEBUG

// Class to represent a dynamic array of unsigned chars, handy for manipulation.
// To optimize things like frequent += <byte>, etc., we allocate more memory than
// actualy needed (the mAllocated indicates the size of this allocation). This is to avoid
// frequent calls of expensive memory reallocations.
// TODO: logic to calculate memory to be reserved should probably be smarter than just using kBytesToAdd, e.g.
// depending on the size of actually requested bytes
// TODO: keep this as close as possible to std::vector for easy migration in case considered needed
class vector
{
public: // Ctor&Dtor
	vector(size_t len = 0);
	vector(const vector & rhs); // Copy ctor allowed
	~vector();

// public: // Variables

public: // Methods
	byte & operator[](std::size_t index) const;

	std::size_t length() const { return mLength; }
	std::size_t allocated() const { return mAllocated; }

	vector & operator<<(const byte val); // Adds one byte to the data
	vector & operator<<(const word val);
	vector & operator<<(const dword & val);
	vector & operator<<(const char * data) { std::size_t len = strlen(data); return append((const byte *)&data[0], (const byte *)&data[len - 1]); }
	vector & operator<<(const vector & data) { return append(&data[0], &data[data.length() - 1]); }
	vector & operator<<(const byte * data) { return append(data, &data[1]); }

	vector & operator=(const byte val);
	vector & operator=(const word val);
	vector & operator=(const dword & val);
	vector & operator=(const char * data) { size_t len = strlen(data); return assign((const byte *)&data[0], (const byte *)&data[len-1]); }
	vector & operator=(const vector & data) { return assign(&data[0], &data[data.length() - 1]); }
	vector & operator=(const std::string & data) { return assign((const byte *)data.data(), data.length()); }

	vector & append(const byte * p1, const byte * p2) { return assign(p1, p2, mLength); }
	vector & append(const byte * data, std::size_t len) { return assign(data, len, mLength); }

	vector & assign(const byte * p1, const byte * p2, std::size_t offset = 0);
	vector & assign(const byte * p1, std::size_t len, std::size_t offset = 0) { return assign(&p1[0], &p1[len - 1], offset); };

	const byte * data() const { return mData; }
//	std::string & ToString();

	// We may have allocated >1Kb and then clear() it to store 10b,
	// keeping 1014b for no reason probably.
	// Not sure if it's good enough, let's look how it behaves.
	void clear() { mLength = 0; }


private: // Variables
	// Add this many bytes for allocation, must be > 0!
	static const std::size_t kBytesToReserve = 5;
	
	byte * mData; // Pointer to the allocated memory
	size_t mLength; // Actual number of inserted bytes
	size_t mAllocated; // Actual allocated space, requested + kBytesToReserve

private: // Methods
//	void log(const char * s) const;
};
#endif // #ifndef VECTOR_HPP
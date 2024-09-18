#include "TapeReader.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"
#include <iostream>

bool TapeReader::readBytes(Bytes& data, int n, int& read_bytes)
{
	for (read_bytes = 0; read_bytes < n; read_bytes++) {
		Byte b;
		if (!readByte(b)) {
			if (mTracing)
				cout << "Failed to read byte #"  << read_bytes << " out of " << n << " bytes\n";
			return false;
		}
		data.push_back(b);
	}
	return true;
}

// Read n bytes if possible
bool TapeReader::readBytes(Bytes &data, int n)
{
	int read_bytes;
	return readBytes(data, n, read_bytes);
}

// Read min nMin, and up to nMax bytes, and until a terminator is encountered
bool TapeReader::readString(string &s, int nMin, int nMax, Byte terminator, int &n)
{
	Byte b = 0x0;
	n = 0;
	s = "";
	do {
		if (!readByte(b)) {
			if (mTracing)
				cout << "Failed to read string terminated with 0x" << hex << terminator  << "\n";
			return false;
		}
		else if (b != terminator)
			s += (char) b;
		n++;
	} while (b != terminator && n <= nMax);

	return (n > 0 && b == terminator);
}
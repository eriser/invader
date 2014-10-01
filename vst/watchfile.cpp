#include "pch.h"
#include "watchfile.h"

ZWatchFile::ZWatchFile(const char* path)
	: data(nullptr)
	//, path(zstrdup(path)) // we duplicate the string, because it might come in from Lua, where it would be garbagecollected
	, path(path) 
	, readTime(-1) // when was the file last read?
{
	//hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ftLastRead.dwLowDateTime = 0;
	ftLastRead.dwHighDateTime = 0;
}


ZWatchFile::~ZWatchFile(void)
{
	//CloseHandle(hFile);
}

bool ZWatchFile::DidUpdate(void)
{
	FILETIME ftCreate, ftAccess, ftWrite;

	hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile==INVALID_HANDLE_VALUE)
		return false; // we'll pick it up next time

	// Retrieve the file times for the file.
	GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);

	if (CompareFileTime(&ftWrite, &ftLastRead)==1)
	{
		// Move to start of file
		SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

		// Get size and realloc
		LARGE_INTEGER size;
		GetFileSizeEx(hFile, &size);

		if (data)
			zalignedfree(data);

		data = (char*)zalignedalloc(size.LowPart+1, 4); // ensure 4 byte align
		((char*)data)[size.LowPart] = '\0'; // append zero for easy text handling

		// Read file
		DWORD  dwBytesRead = 0;

		ReadFile(hFile, data, size.LowPart, &dwBytesRead, NULL);

		// Update read times
		ftLastRead.dwLowDateTime = ftWrite.dwLowDateTime;
		ftLastRead.dwHighDateTime = ftWrite.dwHighDateTime;

		CloseHandle(hFile);
		return true;
	}

	CloseHandle(hFile);
	return false;
}
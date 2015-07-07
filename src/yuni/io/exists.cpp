#include "io.h"
#include "../core/system/windows.hdr.h"
#include "../core/string.h"
#include "../core/string/wstring.h"
#include <sys/stat.h>

#ifndef YUNI_OS_WINDOWS
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
#endif

#ifdef YUNI_HAS_STRING_H
# include <string.h>
#endif





namespace Yuni
{
namespace IO
{

	namespace // anonymous
	{

		static inline NodeType Stat(const AnyString& filename, yuint64& outSize, yint64& lastModified, bool followSymLink)
		{
			if (filename.empty())
				return Yuni::IO::typeUnknown;

			# ifdef YUNI_OS_WINDOWS
			{
				(void) followSymLink; // unused on windows

				const char* p = filename.c_str();
				AnyString::Size len = filename.size();

				// remove the final slash if any
				if (p[len - 1] == '\\' or p[len - 1] == '/')
				{
					if (0 == --len)
						return Yuni::IO::typeUnknown;
				}

				// Driver letters
				if (len == 2 and p[1] == ':' and String::IsAlpha(p[0]))
					return Yuni::IO::typeFolder;

				String norm;
				Yuni::IO::Normalize(norm, AnyString(p, len));

				// Conversion into wchar_t
				Private::WString<true> wstr(norm);
				if (not wstr.empty())
				{
					WIN32_FILE_ATTRIBUTE_DATA infoFile;
					if (0 != GetFileAttributesExW(wstr.c_str(), GetFileExInfoStandard, &infoFile))
					{
						LARGE_INTEGER size;
						size.HighPart = infoFile.nFileSizeHigh;
						size.LowPart  = infoFile.nFileSizeLow;
						outSize = size.QuadPart;
						lastModified = Yuni::Windows::FILETIMEToTimestamp(infoFile.ftLastWriteTime);

						return ((infoFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
							? Yuni::IO::typeFolder
							: Yuni::IO::typeFile;
					}
				}
			}
			# else // WINDOWS
			{
				struct stat s;
				int ret = (followSymLink) ? stat(filename.c_str(), &s) : lstat(filename.c_str(), &s);
				if (0 == ret)
				{
					lastModified = s.st_mtime;
					outSize = (uint64) s.st_size;

					if (0 != S_ISREG(s.st_mode))
						return Yuni::IO::typeFile;
					if (0 != S_ISDIR(s.st_mode))
						return Yuni::IO::typeFolder;
					if (0 != S_ISLNK(s.st_mode))
						return Yuni::IO::typeSymlink;
					if (0 != S_ISSOCK(s.st_mode))
						return Yuni::IO::typeSocket;

					return Yuni::IO::typeSpecial;
				}
			}
			# endif

			return Yuni::IO::typeUnknown;
		}


	} // anonymous namespace





	NodeType TypeOf(const AnyString& filename, bool followSymLink)
	{
		yuint64 size; // useless
		yint64 lastModified;
		return Stat(filename, size, lastModified, followSymLink);
	}


	NodeType FetchFileStatus(const AnyString& filename, yuint64& size, yint64& lastModified, bool followSymLink)
	{
		size = 0u;
		lastModified = 0;
		return Stat(filename, size, lastModified, followSymLink);
	}





} // namespace IO
} // namespace Yuni


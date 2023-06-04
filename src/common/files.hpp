#pragma once
#include <cstdio>
#include <cstring>
#include <string>
// Include OS-dependend stuff for "mkdir".
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace ChasmReverse
{

void FileRead( std::FILE* const file, void* buffer, const unsigned int size );
void FileWrite( std::FILE* const file, const void* buffer, const unsigned int size );
const char* ExtractExtension( const char* const file_path );


} // namespace ChasmReverse

template<class T>
T dir_name( T const& path, T const& delims = "/\\" )
{
	return path.substr(0, path.find_last_of(delims) + 1);
}

template<class T>
T base_name( T const& path, T const& delims = "/\\" )
{
	return path.substr(path.find_last_of(delims) + 1);
}

template<class T>
T remove_extension( T const& filename )
{
	typename T::size_type const p(base_name<std::string>(filename).find_last_of('.'));
	return ((p > 0 && p != T::npos) ? dir_name<std::string>(filename) + "/" + base_name<std::string>(filename).substr(0, p) : filename);
}

template<class T>
T extract_extension( T const& filename )
{
	typename T::size_type const p(base_name<std::string>(filename).find_last_of('.'));
	return ((p > 0 && p != T::npos) ? dir_name<std::string>(filename) + "/" + base_name<std::string>(filename).substr(p, T::npos) : filename);
}

bool exists( const std::string& file_path );
bool is_directory( const std::string& file_path );
bool is_regular_file( const std::string& file_path );
bool is_symlink( const std::string& file_path );
bool real_path( std::string& path );

template<class T>
bool do_mkdir( T const& path )
{
	struct stat sb;
	if(::stat(path.c_str(), &sb) != 0)
	{
#ifdef _WIN32
		if((_mkdir(path.c_str()) != 0) && errno != EEXIST)
#else
		if((mkdir(path.c_str(), 0777) != 0) && errno != EEXIST)
#endif
			return false;
	}
	else if(!S_ISDIR(sb.st_mode))
	{
		errno = ENOTDIR;
		return false;
	}
	return true;
}

template<class T>
bool create_directories( T& path, T const& delims = "/\\" )
{
	T build;
	for(typename T::size_type pos = 0; (pos = path.find('/')) != T::npos;)
	{
		build += path.substr(0, pos + 1);
		do_mkdir<T>(build);
		path.erase(0, pos + 1);
	}
	if (!path.empty()) {
		build += path;
		do_mkdir<T>(build);
	}
	return true;
}

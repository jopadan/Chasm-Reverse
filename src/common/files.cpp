#include "files.hpp"
#include <cstring>
#include <cstdlib>

namespace ChasmReverse
{

void FileRead( std::FILE* const file, void* buffer, const unsigned int size )
{
	unsigned int read_total= 0u;

	do
	{
		const int read= std::fread( static_cast<char*>(buffer) + read_total, 1, size - read_total, file );
		if( read <= 0 )
			break;

		read_total+= read;
	} while( read_total < size );
}

void FileWrite( std::FILE* const file, const void* buffer, const unsigned int size )
{
	unsigned int write_total= 0u;

	do
	{
		const int write= std::fwrite( static_cast<const char*>(buffer) + write_total, 1, size - write_total, file );
		if( write <= 0 )
			break;

		write_total+= write;
	} while( write_total < size );
}

const char* ExtractExtension( const char* const file_path )
{
	unsigned int pos= std::strlen( file_path );

	while( pos > 0u && file_path[ pos ] != '.' )
		pos--;

	return file_path + pos + 1u;
}

} // namespace ChasmReverse

bool exists( const std::string& file_path )
{
	struct stat sb;
	if(stat(file_path.c_str(), &sb) == 0)
		return true;
	return false;
}

bool is_directory( const std::string& file_path )
{
	struct stat sb;
	if((stat(file_path.c_str(), &sb) == 0) && ((sb.st_mode & S_IFMT) == S_IFDIR))
		return true;
	return false;	
}

bool is_regular_file( const std::string& file_path )
{
	struct stat sb;
	if((stat(file_path.c_str(), &sb) == 0) && ((sb.st_mode & S_IFMT) == S_IFREG))
		return true;
	return false;	
}

bool is_symlink( const std::string& file_path )
{
	struct stat sb;
	if((stat(file_path.c_str(), &sb) == 0) && ((sb.st_mode & S_IFMT) == S_IFLNK))
		return true;
	return false;
}

bool real_path( std::string& path )
{
	char* dst = realpath(path.c_str(), NULL);
	if(dst == nullptr)
		return false;

	path = dst;
	free(dst);

	return true;
}


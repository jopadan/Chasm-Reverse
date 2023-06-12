#pragma once
#include <cstdio>
#include <cstring>
#include <string>
#include <filesystem>

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

std::filesystem::path remove_extension( const std::filesystem::path& path );

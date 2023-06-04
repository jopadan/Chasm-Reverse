#include <cctype>
#include <cstring>
#include <unistd.h>

#include "common/files.hpp"
using namespace ChasmReverse;

#include "log.hpp"

#include "save_load.hpp"

#define SAVES_DIR "saves"

namespace PanzerChasm
{

const char SaveHeader::c_expected_id[8]= "PanChSv"; // PanzerChasmSave

static unsigned int g_crc_table[256u];

static void InitCRCTable()
{
	unsigned int crc;

	for(unsigned int i= 0u; i < 256u; i++ )
	{
		crc= i;
		for(unsigned int j= 0u; j < 8u; j++ )
			crc= ( ( crc & 1u ) != 0u ) ? ( (crc>>1u) ^ 0xEDB88320u ) : ( crc >> 1u );

		g_crc_table[i]= crc;
	}
}

// TODO - init table at compile time.
static const bool g_crc_table_init=
[]
{
	InitCRCTable();
	return true;
} ();

SaveHeader::HashType SaveHeader::CalculateHash( const unsigned char* data, unsigned int data_size )
{
	// Calculate CRC-32 here.

	unsigned int crc= 0xFFFFFFFFu;
	for( unsigned int i= 0u; i < data_size; i++ )
	{
		crc= g_crc_table[ ( crc ^ data[i] ) & 0xFFu ] ^ (crc >> 8u);
	}

	return crc;
}

bool SaveData(
	const char* file_name,
	const SaveComment& save_comment,
	const SaveLoadBuffer& data )
{
	FILE* f= std::fopen( file_name, "wb" );
	if( f == nullptr )
	{
		Log::Warning( "Can not write save \"", file_name, "\"" );
		return false;
	}

	SaveHeader header;
	std::memcpy( header.id, SaveHeader::c_expected_id, sizeof(header.id) );
	header.version= SaveHeader::c_expected_version;
	header.content_size= data.size();
	header.content_hash= SaveHeader::CalculateHash( data.data(), data.size() );

	FileWrite( f, &header, sizeof(SaveHeader) );
	FileWrite( f, save_comment.data(), sizeof(SaveComment) );
	FileWrite( f, data.data(), data.size() );

	std::fclose(f);
	return true;
}

// Returns true, if all ok
bool LoadData(
	const char* file_name,
	SaveLoadBuffer& out_data )
{
	FILE* const f= std::fopen( file_name, "rb" );
	if( f == nullptr )
	{
		Log::Warning( "Can not read save \"", file_name, "\"." );
		return false;
	}

	std::fseek( f, 0, SEEK_END );
	const unsigned int file_size= std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	if( file_size < sizeof(SaveHeader) + sizeof(SaveComment) )
	{
		Log::Warning( "Save file is broken - it is too small." );
		std::fclose(f);
		return false;
	}

	SaveHeader header;
	FileRead( f, &header, sizeof(SaveHeader) );

	if( std::memcmp( header.id, header.c_expected_id, sizeof(header.id) ) != 0 )
	{
		Log::Warning( "Save file is not a PanzerChasm save." );
		std::fclose(f);
		return false;
	}
	if( header.version != SaveHeader::c_expected_version )
	{
		Log::Warning( "Save file has different version." );
		std::fclose(f);
		return false;
	}

	const unsigned int content_size= file_size - ( sizeof(SaveHeader) + sizeof(SaveComment) );
	if( header.content_size != content_size )
	{
		Log::Warning( "Save file has content size, different from actual file size." );
		std::fclose(f);
		return false;
	}

	SaveComment save_comment;
	FileRead( f, save_comment.data(), sizeof(SaveComment) );

	out_data.resize( content_size );
	FileRead( f, out_data.data(), out_data.size() );

	if( header.content_hash != SaveHeader::CalculateHash( out_data.data(), out_data.size() ) )
	{
		out_data.clear();

		Log::Warning( "Save file is broken - saved content hash is different from actual content hash." );
		std::fclose(f);
		return false;
	}

	std::fclose(f);
	return true;
}

bool LoadSaveComment(
	const char* file_name,
	SaveComment& out_save_comment )
{
	FILE* const f= std::fopen( file_name, "rb" );
	if( f == nullptr )
	{
		return false;
	}

	std::fseek( f, 0, SEEK_END );
	const unsigned int file_size= std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	if( file_size < sizeof(SaveHeader) + sizeof(SaveComment) )
	{
		std::fclose(f);
		return false;
	}

	std::fseek( f, sizeof(SaveHeader), SEEK_SET );
	FileRead( f, out_save_comment.data(), sizeof(SaveComment) );

	std::fclose(f);
	return true;
}

void GetSaveFileNameForSlot(
	const unsigned int slot_number,
	char* const out_file_name,
	const unsigned int out_file_name_max_length )
{
	std::snprintf( out_file_name, out_file_name_max_length, SAVES_DIR"/save_%02d.pcs", slot_number );
}

std::string GetScreenshotFileNameForDir(const std::string& dir)
{
	static uint8_t slot_number = 0;
	char* str = nullptr;

	// TODO: use list directory to determine slot_number
	ssize_t len = asprintf( &str, "%s/cshot_%02d.tga", dir.empty() ? SAVES_DIR : dir.c_str(), slot_number );
	if(len < 14)
	{
		free(str);
		Log::Warning( "Couldn't allocate memory for screenshot name: ", errno, " - ", strerror(errno));
		return std::string(); 
	}

	slot_number = slot_number < 99 ? slot_number + 1 : 0;
	std::string dst(str, len);

	free(str);
	return dst;
}

std::string CreateScreenshotsDir(const std::string& file)
{
	std::string dst_dir  = dir_name<std::string>(file);
	std::string dst_file = base_name<std::string>(file);
	std::string cwd = get_current_dir_name();

	if(dst_dir.empty())
		dst_dir = file[0] == '/' ? "/" : std::string(get_current_dir_name()) + "/" + SAVES_DIR;

	if((!exists( dst_dir ) && !create_directories<std::string>( dst_dir )) || !real_path( dst_dir ))
	{
		Log::Warning("Couldn't create screenshot directory: ", dst_dir, " - ", strerror(errno));
		dst_dir.clear();
	}

	return dst_dir;
}

void CreateSlotSavesDir()
{
	std::string dir(SAVES_DIR);
	if(!create_directories<std::string>( dir ))
		Log::Warning("Couldn't create saves directory: ", strerror(errno));
}

} // namespace PanzerChasm

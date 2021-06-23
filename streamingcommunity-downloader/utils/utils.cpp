#include "utils.hh"
#include <regex>
#include <curl/curl.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hh>

std::string utils::html_decode( const std::string& in ) {
	auto matched = std::regex_replace( in, std::regex( R"(&quot;)" ), R"(")" );
	matched = std::regex_replace( matched, std::regex( R"(&amp;)" ), "&" );
	matched = std::regex_replace( matched, std::regex( R"(&lt;)" ), "<" );
	matched = std::regex_replace( matched, std::regex( R"(&gt;)" ), ">" );

	return matched;
}

size_t write_data( void* ptr, const size_t size, const size_t nmemb, FILE* stream ) {
	return fwrite( ptr, size, nmemb, stream );
}

void utils::download_from_url( const std::string& url, std::ofstream path ) {
	Download( path, cpr::Url{ url } );
}

std::vector<std::pair<std::string, int>> utils::search_movie( const std::string& movie_name ) {
	auto search_json = nlohmann::json( );
	search_json["q"] = movie_name;
	const auto request = Post( cpr::Url{ R"(https://streamingcommunity.tv/api/search)" }, cpr::Body{ search_json.dump( ) }, cpr::Header{ { "content-type", "application/json;charset=UTF-8" }, { "referer", R"(https://streamingcommunity.tv/search?q=)" } } );
	if ( request.error.code != cpr::ErrorCode::OK || !nlohmann::json::accept( request.text ) )
		return { };

	std::vector<std::pair<std::string, int>> ret;
	auto parsed_request = nlohmann::ordered_json::parse( request.text );
	for ( const auto& item : parsed_request["records"].items( ) )
		ret.emplace_back( std::make_pair( item.value( )["slug"].get<std::string>( ), item.value( )["id"].get<int>( ) ) );

	std::ranges::reverse( ret );
	return ret;
}

std::string utils::generate_token( ) {
	const auto o = 48;
	const auto i = std::string( "Yc8U6r8KjAKAepEA" );
	const auto a = Get( cpr::Url{ "https://api64.ipify.org" } ).text;

	const std::chrono::system_clock::time_point tp = std::chrono::system_clock::now( );
	const std::chrono::system_clock::duration dtn = tp.time_since_epoch( );
	const auto sec_since_epoch = dtn.count( ) * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;

	auto l = sec_since_epoch + 36e5 * o;
	l = std::round( sec_since_epoch + 36e5 * o / 1e3 );
	const auto s = std::format( "{}{} {}", l, a, i );

	const auto bytes = reinterpret_cast<const BYTE*>( &s[0] );
	const DWORD byteLength = s.size( ) * sizeof s[0];

	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	CryptAcquireContextW( &hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT );
	CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash );

	CryptHashData( hHash, bytes, byteLength, 0 );
	BYTE hashBytes[128 / 8];
	DWORD paramLength = sizeof hashBytes;
	CryptGetHashParam( hHash, HP_HASHVAL, hashBytes, &paramLength, 0 );
	CryptDestroyHash( hHash );
	CryptReleaseContext( hProv, 0 );

	DWORD base64Length = 0;
	CryptBinaryToStringA( hashBytes, paramLength, CRYPT_STRING_BASE64, nullptr, &base64Length );
	const auto base64 = new char[base64Length];
	CryptBinaryToStringA( hashBytes, paramLength, CRYPT_STRING_BASE64, base64, &base64Length );

	auto b64 = std::string( base64 );
	b64 = std::regex_replace( b64, std::regex( "\\\r\n" ), "" );
	b64 = std::regex_replace( b64, std::regex( "\\=" ), "" );
	b64 = std::regex_replace( b64, std::regex( "\\+" ), "-" );
	b64 = std::regex_replace( b64, std::regex( "\\/" ), "_" );

	return std::format( "token={}&expires={}", b64, l );
}

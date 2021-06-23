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
		return {};

	std::vector<std::pair<std::string, int>> ret;
	auto parsed_request = nlohmann::ordered_json::parse( request.text );
	for ( const auto& item : parsed_request["records"].items( ) )
		ret.emplace_back( std::make_pair( item.value( )["slug"].get<std::string>( ), item.value( )["id"].get<int>( ) ) );

	std::ranges::reverse( ret );
	return ret;
}
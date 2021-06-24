#include <filesystem>
#include <Windows.h>
#include <xorstr.hh>
#include <nlohmann/json.hh>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <ranges>
#include <regex>
#include <cpr/cpr.h>
#include "utils/utils.hh"
#include <indicators/indeterminate_progress_bar.hpp>

int main( ) {
back:
	if ( !std::filesystem::exists( R"(C:\ffmpeg-2021-06-19-git-2cf95f2dd9-full_build)" ) ) {
		using namespace indicators;
		IndeterminateProgressBar bar{
			option::BarWidth{ 50 },
			option::Start{ "[" },
			option::Fill{ "." },
			option::Lead{ "<==>" },
			option::End{ "]" },
			option::End{ " ]" },
			option::PostfixText{ "Downloading ffmpeg" },
			option::ForegroundColor{ Color::green },
			option::FontStyles{ std::vector{ FontStyle::bold } }
		};

		std::ofstream path( R"(C:\downloaded\ffmpeg.rar)", std::ios::out | std::ios::binary );
		Download( path, cpr::Url{ "https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-2021-06-19-git-2cf95f2dd9-full_build.7z" }, cpr::ProgressCallback( [&]( const size_t downloadTotal, const size_t downloadNow, size_t, size_t ) -> bool {
			system( "cls" );
			bar.tick( );
			if ( downloadNow != 0 && downloadTotal != 0 && downloadNow == downloadTotal ) {
				if ( static auto done = false; !done ) {
					bar.mark_as_completed( );

					done = true;
				}
			}

			std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
			return true;
		} ) );

		std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
		path.close( );

		system( R"(""%ProgramFiles%\WinRAR\winrar.exe"" x -ibck C:\downloaded\ffmpeg.rar *.* c:\)" );
		system( "cls" );
	}

	std::string choosen_movie;
	printf( "what movie do you want to watch? \n" );
	std::getline( std::cin, choosen_movie );

	auto searched_movie = utils::search_movie( choosen_movie );
	if ( searched_movie.empty( ) ) {
		printf( "no movies found with that name \n" );
		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		system( "cls" );
		goto back;
	}

	printf( "Movies found with that name: \n" );

	std::map<std::string, std::vector<episodes_t>> serie_test;
	for ( const auto& a : searched_movie ) {
		printf( std::format( "- {}:{} {} \n", a.first.shrug_name, a.first.id, a.first.is_serie ? "(Serie)" : "(Film)" ).c_str( ) );
		if ( a.first.is_serie )
			serie_test[std::to_string( a.first.id )] = a.second;
	}
	
	std::string choosen_movie_id, choosen_serie_id;
	printf( "choose the movie id \n" );
	std::getline( std::cin, choosen_movie_id );

	std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
	system( "cls" );
	
	if ( !serie_test[choosen_movie_id].empty( ) ) {
		printf( "Episodes found: \n" );
		for ( const auto& serie : serie_test[choosen_movie_id] ) {
			printf( std::format( "- Season {}, Episode {} ({}) \n", serie.season, serie.episodes.first, serie.episodes.second ).c_str( ) );
		}
		
		printf( "choose the episode id \n" );
		std::getline( std::cin, choosen_serie_id );
	}
	
	std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
	system( "cls" );

	const auto fs = std::filesystem::path( R"(C:\downloaded\)" );
	if ( exists( fs ) )
		remove_all( fs );

	create_directory( fs );

	const std::regex rx( R"(<video-player response="(.*))" );
	std::smatch matched_rx;
	const auto a = Get( cpr::Url{ std::format( "https://streamingcommunity.tv/watch/{}{}", choosen_movie_id, choosen_serie_id.empty( ) ? "" : std::format( "?e={}", choosen_serie_id ) ) } );
	if ( !std::regex_search( a.text, matched_rx, rx ) )
		return -1;

	auto video_info = matched_rx.str( 0 );
	video_info = std::regex_replace( video_info, std::regex( R"("></video-player>)" ), "" );
	video_info = std::regex_replace( video_info, std::regex( R"(<video-player response=")" ), "" );
	const auto decoded_info = utils::html_decode( video_info );
	if ( !nlohmann::json::accept( decoded_info ) )
		return -1;

	const auto parsed_video_info = nlohmann::json::parse( decoded_info );
	const auto video_download_id = parsed_video_info["video_id"].get<int>( );
	const auto req = Get( cpr::Url{ std::format( "https://streamingcommunity.tv/videos/master/{}/480p?{}", video_download_id, utils::generate_token( ) ) } );
	std::ofstream downloaded_video_info( fs.parent_path( ).string( ).append( "\\info.txt" ) );
	downloaded_video_info << req.text;

	std::ifstream downloaded_video_info_file( fs.parent_path( ).string( ).append( "\\info.txt" ) );
	std::vector<std::string> clips;
	if ( downloaded_video_info_file.good( ) ) {
		std::string str;
		while ( std::getline( downloaded_video_info_file, str ) ) {
			if ( str.find( '#' ) != std::string::npos || str.find( ".ts" ) == std::string::npos )
				continue;

			clips.emplace_back( str );
		}
	}

	if ( clips.empty( ) ) {
		printf( "something went wrong \n" );
		std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
		return -1;
	}

	printf( "started download for the clips \n" );
	static auto counter = 0;
	for ( const auto& clip : clips ) {
		utils::download_from_url( clip, std::ofstream( fs.parent_path( ).string( ).append( std::format( "\\{}.process", ++counter ) ).c_str( ), std::ios::out | std::ios::binary ) );
	}

	printf( "downloaded %i clips, now processing them \n", counter );
	std::map<int, std::string> names;
	for ( const auto& dir_iter : std::filesystem::recursive_directory_iterator( R"(C:\downloaded\)" ) ) {
		if ( dir_iter.path( ).extension( ) != ".process" )
			continue;

		names[std::stoi( dir_iter.path( ).filename( ).replace_extension( ).string( ) )] = dir_iter.path( ).filename( ).string( ).append( "\n" );
	}

	std::ofstream file( "c:\\downloaded\\ffmpeg_ready.txt", std::ios::out | std::ios::binary );
	for ( const auto& val : names | std::views::values ) {
		file << "file " + val;
	}

	file.close( );
	std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

	system( R"(C:\ffmpeg-2021-06-19-git-2cf95f2dd9-full_build\bin\ffmpeg.exe -f concat -i C:\downloaded\ffmpeg_ready.txt -vcodec copy -acodec copy C:\finished_video.mp4)" );

	printf( "finished! movie saved in the c drive! \n" );
	system( "pause" );
	return EXIT_SUCCESS;
}

#pragma once
#include <string>
#include <vector>

namespace utils {
	std::string html_decode( const std::string& );
	void download_from_url( const std::string&, std::ofstream );
	std::vector<std::pair<std::string, int>> search_movie( const std::string& );
	std::string generate_token( );
}

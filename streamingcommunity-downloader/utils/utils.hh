#pragma once
#include <string>
#include <vector>

struct info_t {
	std::string shrug_name;
	int id;
	bool is_serie;
};

struct episodes_t {
	int season;
	std::pair<int, int> episodes;
};

namespace utils {
	std::string html_decode( const std::string& );
	void download_from_url( const std::string&, std::ofstream );
	std::vector<std::pair<info_t, std::vector<episodes_t>>> search_movie( const std::string& );
	std::string generate_token( );
}

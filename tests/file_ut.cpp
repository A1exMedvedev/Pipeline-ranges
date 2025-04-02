#include <processing.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>


std::vector<std::string> SplitString(const std::string& str) {
    std::istringstream iss(str);

    return {std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>()};
}


TEST(FilterTest, SimpleFilterFileTest) {
    std::vector<std::stringstream> current(1);
    Dir("../MyDir", 1) 
        | Filter([](std::filesystem::path &p) { return p.extension() == ".txt"; })
        | Transform(
            [](const std::filesystem::path &p) { return p.string(); })
        | Write(current[0], '/');

    auto result = AsDataFlow(current)
        | Split("/")
        | Filter([](std::string& str) { return (str[str.size() - 1] == 't' && str[str.size() - 2] == 'x' && str[str.size() - 3] == 't'); })
        | AsVector(); 

    ASSERT_THAT(result, testing::ElementsAre("2.txt", "5.txt", "1.txt"));
}


TEST(DirTest, AllMixedTest) {
    auto result = Dir("../MyDir", 1) 
    | Filter([](std::filesystem::path& p){ return p.extension() == ".txt"; })
    | OpenFiles()
    | Split("\n ,.;")
    | Transform(
        [](std::string& token) { 
            std::transform(token.begin(), token.end(), token.begin(), [](char c){return std::tolower(c);});
            return token;
        })
    | AggregateByKey(
        0uz, 
        [](const std::string&, size_t& count) { ++count;},
        [](const std::string& token) { return token;}
      )
    | Transform([](const std::pair<std::string, size_t>& stat) { return std::format("{} - {}", stat.first, stat.second);})
    | Filter([](const std::string& str){ return (str[str.size() - 1] - '0') > 2;})
    | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("Алгоритм - 8", "работает - 6", "через - 8", "dfs - 3"));
}

#include <processing.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(DropNulloptTest, DropNullopt) {
    std::vector<std::optional<int>> input = {1, std::nullopt, 3, std::nullopt, 5};
    auto result = AsDataFlow(input) | DropNullopt() | AsVector();
    ASSERT_THAT(result, testing::ElementsAre(1, 3, 5));
}


TEST(TransformDropNulloptTest, RemoveInvalidNumbers) {
    std::vector<std::string> data = {"10", "abc", "20", "???", "30"};

    auto result = AsDataFlow(data)
    | Transform([](const std::string& str) -> std::optional<int> { 
        try {
            return std::stoi(str);
        } catch (...) {
            return std::nullopt;
        }
      })
    | DropNullopt()
    | Transform([](int num) { return num * 2; })
    | AsVector();

    ASSERT_THAT(result, testing::ElementsAre(20, 40, 60));
}


TEST(TransformDropNulloptTest, RemoveNegativeNumbers) {
    std::vector<int> data = {10, -5, 20, -15, 30};

    auto result = AsDataFlow(data)
    | Transform([](int num) -> std::optional<int> {
        return (num >= 0) ? std::optional<int>(num) : std::nullopt;
      })
    | DropNullopt()
    | Transform([](int num) { return num + 10; })
    | AsVector();

    ASSERT_THAT(result, testing::ElementsAre(20, 30, 40));
}


TEST(TransformDropNulloptTest, RemoveEmptyStrings) {
    std::vector<std::string> data = {"hello", "", "world", "", "!"};

    auto result = AsDataFlow(data)
    | Transform([](const std::string& str) -> std::optional<std::string> {
        return (!str.empty()) ? std::optional<std::string>(str) : std::nullopt;
      })
    | DropNullopt()
    | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("hello", "world", "!"));
}
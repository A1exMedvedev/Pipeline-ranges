#include <exception>
#include <processing.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <expected>

struct Department {
    std::string name;

    bool operator==(const Department& other) const = default;
};

std::expected<Department, std::string> ParseDepartment(const std::string& str) {
    if (str.empty()) {
        return std::unexpected("Department name is empty");
    }
    if (str.contains(' ')) {
        return std::unexpected("Department name contains space");
    }
    return Department{str};
}

TEST(SplitExpectedTest, SplitExpected) {
    std::vector<std::stringstream> files(1);
    files[0] << "good-department|bad department||another-good-department";

    auto [unexpected_flow, good_flow] = AsDataFlow(files) | Split("|") | Transform(ParseDepartment) | SplitExpected();

    std::stringstream unexpected_file;
    unexpected_flow | Write(unexpected_file, '.');

    auto expected_result = good_flow | AsVector();

    ASSERT_EQ(unexpected_file.str(), "Department name contains space.Department name is empty.");
    ASSERT_THAT(expected_result, testing::ElementsAre(Department{"good-department"}, Department{"another-good-department"}));
}


std::expected<int, std::string> ParseInteger(const std::string& str) {
    try {
        int result = std::stoi(str);

        if (result < 0) {
            return std::unexpected("Negative number");
        }
        return result;
    } catch (const std::exception& e) {
        return std::unexpected("Invalid integer");
    }
}


TEST(SplitExpectedTest, SplitValidAndInvalidIntegers) {
    std::vector<std::stringstream> files(1);
    files[0] << "42|-5|100|abc|7";

    auto [invalid_flow, valid_flow] = AsDataFlow(files) | Split("|") | Transform(ParseInteger) | SplitExpected();

    std::stringstream invalid_file;
    invalid_flow | Write(invalid_file, '.');

    auto valid_result = valid_flow | AsVector();

    ASSERT_EQ(invalid_file.str(), "Negative number.Invalid integer.");
    ASSERT_THAT(valid_result, testing::ElementsAre(42, 100, 7));
}





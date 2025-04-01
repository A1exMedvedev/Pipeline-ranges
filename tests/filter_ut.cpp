#include <processing.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>

TEST(FilterTest, FilterEven) {
    std::vector<int> input = {1, 2, 3, 4, 5};
    auto result = AsDataFlow(input) | Filter([](int x) { return x % 2 == 0; }) | AsVector();
    ASSERT_THAT(result, testing::ElementsAre(2, 4));
}

TEST(FilterTest, FilterUpperCase) {
    std::vector<std::string> input = {"hello", "world", "HELLO", "WORLD"};
    auto result =
        AsDataFlow(input)
            | Filter([](const std::string& x) { return std::all_of(x.begin(), x.end(), [](char c) { return std::isupper(c); }); })
            | AsVector();
    ASSERT_THAT(result, testing::ElementsAre("HELLO", "WORLD"));
}


TEST(FilterSplitTest, SplitAndFilterStructs) {
    struct Employee {
        std::string name;
        int age;

        bool operator==(const Employee& other) const  = default;
    };
    
    std::vector<Employee> employees = {
        {"Alice", 25},
        {"Bob", 40},
        {"Charlie", 30},
        {"David", 45}
    };
    
    auto older =
        AsDataFlow(employees)
        | Filter([](const Employee& e) { return e.age > 30; })
        | Filter([](const Employee& e) { return e.name != "David"; });

    auto older_result = older | AsVector();

    ASSERT_THAT(older_result, ::testing::ElementsAre(Employee{"Bob", 40}));
}

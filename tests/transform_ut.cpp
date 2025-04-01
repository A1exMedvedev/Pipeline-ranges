#include <processing.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(TransformTest, PowerOfTwo) {
    std::vector<int> input = {1, 2, 3, 4, 5};
    auto result = AsDataFlow(input) | Transform([](int x) { return x * x; }) | AsVector();
    ASSERT_THAT(result, testing::ElementsAre(1, 4, 9, 16, 25));
}

TEST(TransformTest, FromStringToInt) {
    std::vector<std::stringstream> files(2);
    files[0] << "1 2 3 4 5";
    files[1] << "6 7 8 9 10";
    auto result = AsDataFlow(files) | Split(" ") | Transform([](const std::string& str) { return std::stoi(str); }) | AsVector();
    ASSERT_THAT(result, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}


TEST(TransformTest, ConditionalTransformation) {
    std::vector<int> input = {1, 2, 3, 4, 5};

    auto result = AsDataFlow(input)
                    | Transform([](int x) { return (x % 2 == 0) ? x * 2 : x * 3; })
                    | AsVector();

    ASSERT_THAT(result, ::testing::ElementsAre(3, 4, 9, 8, 15));
}


TEST(TransformTest, MultipleTransformations) {
    std::vector<int> input = {1, 2, 3, 4, 5};

    auto result = AsDataFlow(input)
                    | Transform([](int x) { return x * 2; })
                    | Transform([](int x) { return x + 1; })
                    | AsVector();

    ASSERT_THAT(result, ::testing::ElementsAre(3, 5, 7, 9, 11));
}


TEST(TransformTest, TransformWithStructure) {

    struct Employee {
        std::string name;
        int age;
    };

    std::vector<Employee> employees = {
        {"Alice", 30},
        {"Bob", 40},
        {"Charlie", 25}
    };

    auto result = AsDataFlow(employees)
                    | Transform([](const Employee& emp) { return emp.name + " is " + std::to_string(emp.age); })
                    | AsVector();

    ASSERT_THAT(result, ::testing::ElementsAre("Alice is 30", "Bob is 40", "Charlie is 25"));
}

TEST(TransformTest, ConditionalFilteringDuringTransform) {
    std::vector<int> input = {1, 2, 3, 4, 5};

    auto result = AsDataFlow(input)
                    | Transform([](int x) { return (x % 2 == 0) ? std::optional<int>(x) : std::nullopt; })
                    | DropNullopt()
                    | AsVector();

    ASSERT_THAT(result, ::testing::ElementsAre(2, 4));
}


TEST(TransformTest, ComplexTransformPipeline) {
    std::vector<std::pair<std::string, int>> data = {
        {"Alice", 25},
        {"Bob", 30},
        {"Charlie", 22}
    };

    auto result = AsDataFlow(data)
    | Transform([](const std::pair<std::string, int>& p) { 
        return std::format("{}: {}", p.first, p.second); 
      })
    | Transform([](const std::string& str) { return str + " years old"; })
    | AsVector();

    ASSERT_THAT(result, testing::ElementsAre(
        "Alice: 25 years old",
        "Bob: 30 years old",
        "Charlie: 22 years old"
    ));
}

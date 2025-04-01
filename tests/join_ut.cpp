#include <optional>
#include <processing.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct Student {
    uint64_t group_id;
    std::string name;

    bool operator==(const Student& other) const = default;
};

struct Group {
    uint64_t id;
    std::string name;

    bool operator==(const Group& other) const = default;
};

TEST(SimpleTest, JoinKV) {
    std::vector<KV<int, std::string>> left = {{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}, {1, "e"}};
    std::vector<KV<int, std::string>> right = {{0, "f"}, {1, "g"}, {3, "i"}};

    auto left_flow = AsDataFlow(left);
    auto right_flow = AsDataFlow(right); 
    auto result = left_flow | Join(right_flow) | AsVector();

    ASSERT_THAT(
        result,
        testing::ElementsAre(
            JoinResult<std::string, std::string>{"a", "f"},
            JoinResult<std::string, std::string>{"b", "g"},
            JoinResult<std::string, std::string>{"c", std::nullopt},
            JoinResult<std::string, std::string>{"d", "i"},
            JoinResult<std::string, std::string>{"e", "g"}
        )
    );
}

TEST(SimpleTest, JoinComparators) {
        std::vector<Student> students = {{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}, {1, "e"}};
        std::vector<Group> groups = {{0, "f"}, {1, "g"}, {3, "i"}};

        auto students_flow = AsDataFlow(students);
        auto groups_flow = AsDataFlow(groups);

        auto result =
            students_flow
                | Join(
                    groups_flow,
                    [](const Student& student) { return student.group_id; },
                    [](const Group& group) { return group.id; })
                | AsVector()
        ;

    ASSERT_THAT(
        result,
        testing::ElementsAre(
            JoinResult<Student, Group>{Student{0, "a"}, Group{0, "f"}},
            JoinResult<Student, Group>{Student{1, "b"}, Group{1, "g"}},
            JoinResult<Student, Group>{Student{2, "c"}, std::nullopt},
            JoinResult<Student, Group>{Student{3, "d"}, Group{3, "i"}},
            JoinResult<Student, Group>{Student{1, "e"}, Group{1, "g"}}
        )
    );
}



TEST(JoinTest, JoinsTwoDataFlows) {
    std::vector<KV<int, std::string>> left = {
        {1, "a"},
        {2, "b"},
        {3, "c"}
    };
    std::vector<KV<int, std::string>> right = {
        {1, "A"},
        {2, "B"},
        {4, "D"}
    };

    auto joined = AsDataFlow(left) | Join(AsDataFlow(right))
            | AsVector();

    ASSERT_THAT(
        joined,
        ::testing::ElementsAre(
            JoinResult<std::string, std::string>{ "a", std::make_optional("A") },
            JoinResult<std::string, std::string>{ "b", std::make_optional("B") },
            JoinResult<std::string, std::string>{ "c", std::nullopt }
        )
    );
}


struct ITMOStudent {
    int id;
    std::string name;
    
    bool operator==(const ITMOStudent& other) const = default;
};

struct Course {
    int student_id;
    std::string course_name;

    bool operator==(const Course& other) const = default;
};

struct Grade {
    int student_id;
    int grade;

    bool operator==(const Grade& other) const = default;
};


TEST(JoinTest, TripleJoin) {
    std::vector<ITMOStudent> students = {{1, "Alice"}, {2, "Bob"}, {3, "Charlie"}};
    std::vector<Course> courses = {{1, "Math"}, {1, "Physics"}, {2, "Biology"}};
    std::vector<Grade> grades = {{1, 90}, {1, 85}, {2, 78}, {3, 92}};

    auto student_courses =
        AsDataFlow(students)
        | Join(
            AsDataFlow(courses),
            [](const ITMOStudent& s) { return s.id; },
            [](const Course& c) { return c.student_id; });

    

    auto result =
        student_courses
        | Join(
            AsDataFlow(grades),
            [](const JoinResult<ITMOStudent, Course>& sc) { return sc.base.id; },
            [](const Grade& g) { return g.student_id; })
        | AsVector();

    ASSERT_THAT(
        result,
        testing::ElementsAre(
            JoinResult<JoinResult<ITMOStudent, Course>, Grade>{
                JoinResult<ITMOStudent, Course>{ITMOStudent{1, "Alice"}, Course{1, "Math"}}, Grade{1, 90}},
            JoinResult<JoinResult<ITMOStudent, Course>, Grade>{
                JoinResult<ITMOStudent, Course>{ITMOStudent{1, "Alice"}, Course{1, "Math"}}, Grade{1, 85}},
            JoinResult<JoinResult<ITMOStudent, Course>, Grade>{
                JoinResult<ITMOStudent, Course>{ITMOStudent{1, "Alice"}, Course{1, "Physics"}}, Grade{1, 90}},
            JoinResult<JoinResult<ITMOStudent, Course>, Grade>{
                JoinResult<ITMOStudent, Course>{ITMOStudent{1, "Alice"}, Course{1, "Physics"}}, Grade{1, 85}},
            JoinResult<JoinResult<ITMOStudent, Course>, Grade>{
                JoinResult<ITMOStudent, Course>{ITMOStudent{2, "Bob"}, Course{2, "Biology"}}, Grade{2, 78}},
            JoinResult<JoinResult<ITMOStudent, Course>, Grade>{
                JoinResult<ITMOStudent, Course>{ITMOStudent{3, "Charlie"}, std::nullopt}, Grade{3, 92}}
        )
    );
}
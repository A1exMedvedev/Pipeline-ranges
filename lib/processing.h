#pragma once

#include <iostream>
#include <variant>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <fstream>
#include <cstddef>
#include <expected>
#include <utility>
#include <format>

class Dir {
private:
    std::variant<std::filesystem::directory_iterator, std::filesystem::recursive_directory_iterator> iter_;
    std::filesystem::path path_;
    bool recursive_;

public:
    class Iterator {
    private:
        std::variant<std::filesystem::directory_iterator, std::filesystem::recursive_directory_iterator> current_;

    public:
        Iterator(bool recursive, const std::filesystem::path &path) {
            if (recursive) {
                current_.emplace<1>(path);
            } else {
                current_.emplace<0>(path);
            }
        }

        Iterator(bool recursive) {
            if (recursive) {
                current_.emplace<1>();
            } else {
                current_.emplace<0>();
            }
        }

        Iterator &operator++() {
            std::visit([&](auto &it) { ++it; }, current_);
            return *this;
        }

        auto &operator*() const {
            return std::visit([&](auto &it) -> std::filesystem::path &{
                static std::filesystem::path p;
                p = it->path();
                return p;
            }, current_);
        }

        bool operator!=(const Iterator &other) const {
            return current_ != other.current_;
        }

        auto path() const {
            return std::visit([&](auto &&it) { return it->path(); }, current_);
        }
    };

public:
    Dir(std::filesystem::path path, bool recursive = false) {
        path_ = path;
        recursive_ = recursive;
    }

    Iterator begin() const { return Iterator(recursive_, path_); }
    Iterator end() const { return Iterator(recursive_); }
};


template<typename Predicate>
class Filter {
public:
    template<typename Iter>
    class Iterator {
        Iter iter_;
        Iter end_;
        Predicate pred_;

        void filter() {
            while (iter_ != end_ && !pred_(*iter_)) {
                ++iter_;
            }
        }

    public:
        Iterator(Iter begin, Iter end, Predicate pred)
            : iter_(begin), end_(end), pred_(pred) {
            filter();
        }

        Iterator &operator++() {
            ++iter_;
            filter();
            return *this;
        }

        const auto operator*() { return *iter_; }

        const auto *operator->() { return &iter_; }

        bool operator!=(const Iterator &other) const {
            return iter_ != other.iter_;
        }

        Iterator begin() const { return *this; }
        Iterator end() const { return Iterator(end_, end_, pred_); }
    };

    Filter(Predicate pred) : pred_(std::move(pred)) {
    }

    template<typename DataFlow>
    auto operator()(DataFlow &&data) const {
        return Iterator(data.begin(), data.end(), pred_);
    }

private:
    Predicate pred_;
};

class OpenFiles {
public:
    template<typename Iter>
    class Iterator {
        Iter iter_;
        Iter end_;
        std::shared_ptr<std::ifstream> current_file_;

    public:
        Iterator(Iter begin, Iter end)
            : iter_(begin), end_(end) {
            if (iter_ != end_) {
                current_file_ = std::make_shared<std::ifstream>();
                current_file_->open(iter_->path());
            }
        }

        Iterator &operator++() {
            current_file_->close();
            ++iter_;
            if (iter_ != end_) {
                current_file_->open(iter_->path());
            }
            return *this;
        }

        std::ifstream &operator*() {
            return *current_file_;
        }

        bool operator!=(const Iterator &other) const {
            return iter_ != other.iter_;
        }

        Iterator begin() { return Iterator(iter_, end_); }
        Iterator end() { return Iterator(end_, end_); }
    };

    template<typename DataFlow>
    auto operator()(DataFlow &&data) const {
        return Iterator(data.begin(), data.end());
    }
};


class Split {
private:
    std::string split_str_;

public:
    template<typename Iter>
    class Iterator {
    private:
        Iter iter_;
        Iter end_;
        std::string split_symbols_;

        std::string current_word_ = "";

        std::istreambuf_iterator<char> file_iter_;
        std::istreambuf_iterator<char> end_iter_;

        int flag = 1;

        void split() {
            current_word_ = "";
            if (!(iter_ != end_)) {
                return;
            }
            while (file_iter_ != end_iter_) {
                char c = *file_iter_;
                ++file_iter_;
                if (split_symbols_.find(c) == std::string::npos) {
                    current_word_ += c;
                } else {
                    return;
                }
            }

            (*iter_).clear();
            (*iter_).seekg(0, std::ios::beg);

            ++iter_;
            if (iter_ != end_) {
                file_iter_ = std::istreambuf_iterator<char>(*iter_);
            }
            return;
        }

    public:
        Iterator(Iter begin, Iter end, const std::string &split_symbols)
            : iter_(begin), end_(end), split_symbols_(split_symbols),
              file_iter_(iter_ != end_
                             ? std::istreambuf_iterator<char>(*iter_)
                             : std::istreambuf_iterator<char>()) {
            split_symbols_ += '\0';
        }

        Iterator &operator++() {
            split();
            return *this;
        }

        std::string &operator*() {
            if (current_word_ == "" && flag) split();
            flag = 0;
            return current_word_;
        }

        bool operator!=(const Iterator &other) const {
            return iter_ != other.iter_ || current_word_ != other.current_word_;
        }

        Iterator begin() { return Iterator(iter_, end_, split_symbols_); }
        Iterator end() { return Iterator(end_, end_, split_symbols_); }
    };

    Split(const std::string &str) : split_str_(str) {
    }

    template<typename DataFlow>
    auto operator()(DataFlow &&data) const {
        return Iterator(data.begin(), data.end(), split_str_);
    }
};


class Out {
    std::ostream &os_;

public:
    Out(std::ostream &os = std::cout) : os_(os) {
    }

    template<typename Iter>
    void operator()(Iter &&iter) {
        for (auto item: iter) {
            os_ << item << std::endl;
        }
    }
};


template<typename Predicate>
class Transform {
private:
    Predicate pred_;

public:
    template<typename Iter>
    class Iterator {
    private:
        Iter iter_;
        Iter end_;
        Predicate predicate_;

    public:
        Iterator(Iter begin, Iter end, Predicate pred) : iter_(begin), end_(end), predicate_(pred) {
        }

        Iterator &operator++() {
            ++iter_;
            return *this;
        }

        auto operator*() {
            return predicate_(*iter_);
        }

        bool operator!=(const Iterator &other) const {
            return iter_ != other.iter_;
        }

        Iterator begin() { return Iterator(iter_, end_, predicate_); }
        Iterator end() { return Iterator(end_, end_, predicate_); }
    };

    Transform(Predicate pred) : pred_(pred) {
    }

    template<typename DataFlow>
    auto operator()(DataFlow &&data) const {
        return Iterator(data.begin(), data.end(), pred_);
    }
};


template<typename InitValue, typename Rredicate, typename KeyFunc>
class AggregateByKey {
private:
    KeyFunc key_func_;
    Rredicate predicate_;
    InitValue init_value_;

public:
    AggregateByKey(InitValue init_value, Rredicate predicate, KeyFunc key_func)
        : init_value_(init_value), predicate_(predicate), key_func_(key_func) {
    }

    template<typename DataFlow>
    auto operator()(DataFlow &&data) {
        std::unordered_map<decltype(key_func_(*data.begin())), std::pair<InitValue, size_t> > data_;
        size_t index = 0;
        for (const auto &item: data) {
            if (data_.find(key_func_(item)) == data_.end()) {
                data_[key_func_(item)].first = init_value_;
                data_[key_func_(item)].second = index;
                index++;
            }
            predicate_(item, data_[key_func_(item)].first);
        }

        std::vector<std::pair<decltype(key_func_(*data.begin())), InitValue> > new_flow_(data_.size());
        for (auto item: data_) {
            new_flow_[item.second.second] = {item.first, item.second.first};
        }
        return new_flow_;
    }
};


template<typename Container>
class AsDataFlow {
private:
    Container &container_;

public:
    AsDataFlow(Container &container) : container_(container) {
    }

    using value_type = Container::value_type;

    auto begin() const { return container_.begin(); }
    auto end() const { return container_.end(); }
};


class AsVector {
public:
    template<typename DataFlow>
    auto operator()(DataFlow &&data) {
        using ValueType = std::decay_t<decltype(*data.begin())>;
        std::vector<ValueType> data_;
        for (const auto &item: data) {
            data_.push_back(item);
        }
        return data_;
    }
};


class DropNullopt {
public:
    template<typename Iter>
    class Iterator {
        Iter iter_;
        Iter end_;

        void skip_nullopt() {
            while (iter_ != end_ && !(*iter_).has_value()) {
                ++iter_;
            }
        }

    public:
        Iterator(Iter begin, Iter end)
            : iter_(begin), end_(end) {
            skip_nullopt();
        }

        Iterator &operator++() {
            ++iter_;
            skip_nullopt();
            return *this;
        }

        auto operator*() {
            return (*iter_).value();
        }

        bool operator!=(const Iterator &other) const {
            return iter_ != other.iter_;
        }

        Iterator begin() { return *this; }
        Iterator end() { return Iterator(end_, end_); }
    };

    template<typename DataFlow>
    auto operator()(DataFlow &&data) const {
        return Iterator(data.begin(), data.end());
    }
};


class Write {
    std::ostream &os_;

    const char delimiter_;

public:
    Write(std::ostream &os = std::cout, const char delimiter = ' ') : os_(os), delimiter_(delimiter) {
    }

    template<typename Iter>
    auto operator()(Iter &&iter) {
        for (auto item: iter) {
            os_ << item << delimiter_;
        }
        return iter;
    }
};


template<typename Key, typename Value>
struct KV {
    Key key;
    Value value;

    bool operator==(const KV<Key, Value> &other) const = default;
};

template<typename Base, typename Joined>
struct JoinResult {
    Base base;
    std::optional<Joined> joined;

    bool operator==(const JoinResult<Base, Joined> &other) const = default;
};


template<typename Right, typename LeftFunc, typename RightFunc>
class JoinFunc {
private:
    Right right_flow_;
    LeftFunc left_func_;
    RightFunc right_func_;

public:
    JoinFunc(Right right_flow, LeftFunc left_func, RightFunc right_func)
        : right_flow_(right_flow),
          left_func_(left_func),
          right_func_(right_func) {
    }


    template<typename DataFlow>
    auto operator()(DataFlow &&left_flow) const {
        using left_type = std::decay_t<decltype(*left_flow.begin())>;
        using right_type = std::decay_t<decltype(*right_flow_.begin())>;
        using result_type = JoinResult<left_type, right_type>;

        std::vector<result_type> result;
        for (const auto &item_l: left_flow) {
            bool flag = false;
            for (const auto &item_r: right_flow_) {
                if (left_func_(item_l) == right_func_(item_r)) {
                    result.push_back({item_l, item_r});
                    flag = true;
                }
            }
            if (!flag) {
                result.push_back({item_l, std::nullopt});
            }
        }
        return result;
    }
};


template<typename Right, typename LeftKeyFunc, typename RightKeyFunc>
auto Join(Right right_range, LeftKeyFunc left_extractor, RightKeyFunc right_extractor) {
    return JoinFunc<Right, LeftKeyFunc, RightKeyFunc>(
        right_range,
        left_extractor,
        right_extractor
    );
}


template<typename Right>
class JoinSimple {
private:
    Right right_flow_;

public:
    JoinSimple(Right right_flow) : right_flow_(right_flow) {
    }

    template<typename DataFlow>
    auto operator()(DataFlow &&left_flow) {
        using left_type = std::decay_t<decltype((*left_flow.begin()).value)>;
        using right_type = std::decay_t<decltype((*right_flow_.begin()).value)>;
        using result_type = JoinResult<left_type, right_type>;

        std::vector<result_type> result;
        for (const auto &item_l: left_flow) {
            bool flag = false;
            for (const auto &item_r: right_flow_) {
                if (item_l.key == item_r.key) {
                    result.push_back({item_l.value, item_r.value});
                    flag = true;
                }
            }
            if (!flag) {
                result.push_back({item_l.value, std::nullopt});
            }
        }
        return result;
    }
};


template<typename Right>
auto Join(Right right_range) {
    return JoinSimple(right_range);
}


class SplitExpected {
public:
    template<typename Iter>
    class ExpectedIterator {
    private:
        Iter iter_;
        Iter end_;

        void apdate() {
            ++iter_;
            if (!(iter_ != end_)) {
                return;
            }
            while (!(*iter_).has_value()) {
                ++iter_;
                if (!(iter_ != end_)) {
                    return;
                }
            }
        }

    public:
        ExpectedIterator(Iter iter, Iter end) : iter_(iter), end_(end) {
        }

        ExpectedIterator &operator++() {
            apdate();
            return *this;
        }

        auto operator*() {
            if (!(*iter_).has_value()) apdate();

            return (*iter_).value();
        }

        bool operator!=(const ExpectedIterator &other) const {
            return iter_ != other.iter_;
        }

        ExpectedIterator begin() const {
            return ExpectedIterator(iter_, end_);
        }

        ExpectedIterator end() const {
            return ExpectedIterator(end_, end_);
        }
    };

    template<typename Iter>
    class UnexpectedIterator {
    private:
        Iter iter_;
        Iter end_;

        void apdate() {
            ++iter_;
            if (!(iter_ != end_)) {
                return;
            }
            while ((*iter_).has_value()) {
                ++iter_;
                if (!(iter_ != end_)) {
                    return;
                }
            }
        }

    public:
        UnexpectedIterator(Iter iter, Iter end) : iter_(iter), end_(end) {
        }

        UnexpectedIterator &operator++() {
            apdate();
            return *this;
        }

        auto operator*() {
            if ((*iter_).has_value()) apdate();
            return (*iter_).error();
        }

        bool operator!=(const UnexpectedIterator &other) const {
            return iter_ != other.iter_;
        }

        UnexpectedIterator begin() const {
            return UnexpectedIterator(iter_, end_);
        }

        UnexpectedIterator end() const {
            return UnexpectedIterator(end_, end_);
        }
    };


    template<typename Flow>
    auto operator()(Flow &&flow) const {
        auto expected = ExpectedIterator(flow.begin(), flow.end());
        auto unexpected = UnexpectedIterator(flow.begin(), flow.end());

        return std::make_pair(unexpected, expected);
    }
};


template<typename T, typename F>
    requires requires(T &&data, F &&fn) { fn(data); }
auto operator|(T &&data, F &&fn) {
    return fn(data);
}

#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <sstream>

template <typename T>
std::ostream& operator<< (std::ostream& stream, const std::vector<T> & vec) {
    stream << "[";
    for (int i = 0 ; i < vec.size () ;i++) {
        if (i != 0) stream << ", ";
        stream << vec[i];
    }
    stream << "]";
    return stream;
}

std::ostream& operator<< (std::ostream& stream, const std::vector<char> & vec);

template <typename K, typename V>
std::ostream& operator<<(std::ostream& stream, const std::map <K, V>& map) {
    stream << "{";
    int i = 0;
    for (auto & it : map) {
        if (i != 0) stream << ", ";
        stream << it.first << " : " << it.second;
        i += 1;
    }
    stream << "}";
    return stream;
}

template <typename K>
std::ostream& operator<<(std::ostream& stream, const std::set <K>& map) {
    stream << "{";
    int i = 0;
    for (auto & it : map) {
        if (i != 0) stream << ", ";
        stream << it;
        i += 1;
    }
    stream << "}";
    return stream;
}


template <typename K, typename V>
std::ostream& operator<<(std::ostream& stream, const std::pair <K, V>& p) {
    stream << "{";
    stream << p.first << "," << p.second;
    stream << "}";
    return stream;
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& stream, const std::unordered_map <K, V>& map) {
    stream << "{";
    int i = 0;
    for (auto & it : map) {
        if (i != 0) stream << ", ";
        stream << it.first << " : " << it.second;
        i += 1;
    }
    stream << "}";
    return stream;
}


namespace common::utils {
    std::string strip_string (const std::string & s);
    std::string findAndReplaceAll(const std::string & data, const std::string & toSearch, const std::string & replaceStr);

    std::vector<std::string> split_string (const std::string & s, const std::string & splitter);

    bool is_prefix (const std::string & s, const std::string & of);


    std::string duration_format (float duration);


}


#pragma once
#include <array>
#include <string>
#include <string_view>

namespace StringEscaping {
    static constexpr std::array rules_storage{
        std::pair{'\n', 'n'}, std::pair{'\r', 'r'}, std::pair{'\t', 't'},
        std::pair{'\v', 'v'}, std::pair{'\f', 'f'}, std::pair{' ', 's'},
    };

    template <size_t N = rules_storage.size(), std::array<std::pair<char, char>, N> Rules = rules_storage>
    [[nodiscard]] static std::string encodeString(std::string_view s) {
        std::string r;
        r.reserve(s.size());
        for (const char c : s) {
            bool replaced = false;

            [&]<size_t... Is>(std::index_sequence<Is...>) {
                ((c == Rules[Is].first ? (r.append(1, '\\').push_back(Rules[Is].second), replaced = true)
                                       : false) ||
                 ...);
            }(std::make_index_sequence<N>{});

            if (!replaced) {
                r.push_back(c);
            }
        }
        return r;
    }

    template <size_t N = rules_storage.size(), std::array<std::pair<char, char>, N> Rules = rules_storage>
    [[nodiscard]] static std::string decodeString(std::string_view s) {
        std::string r;
        r.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                bool replaced = false;
                const char next = s[i + 1];

                [&]<size_t... Is>(std::index_sequence<Is...>) {
                    ((next == Rules[Is].second ? (r.push_back(Rules[Is].first), replaced = true, ++i)
                                               : false) ||
                     ...);
                }(std::make_index_sequence<N>{});

                if (!replaced) {
                    r.push_back('\\');
                }
            }
            else {
                r.push_back(s[i]);
            }
        }
        return r;
    }
}
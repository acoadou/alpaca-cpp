#pragma once

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "alpaca/Exceptions.hpp"

namespace alpaca {

/// Container preserving HTTP header casing while supporting case-insensitive lookup
/// and duplicate values.
class HttpHeaders {
  public:
    using value_type = std::pair<std::string, std::string>;
    using container_type = std::vector<value_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    HttpHeaders() = default;
    HttpHeaders(std::initializer_list<value_type> init) {
        for (auto const& entry : init) {
            append(entry.first, entry.second);
        }
    }

    [[nodiscard]] bool empty() const noexcept {
        return entries_.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return entries_.size();
    }

    void clear() noexcept {
        entries_.clear();
    }

    iterator begin() noexcept {
        return entries_.begin();
    }
    iterator end() noexcept {
        return entries_.end();
    }
    const_iterator begin() const noexcept {
        return entries_.begin();
    }
    const_iterator end() const noexcept {
        return entries_.end();
    }
    const_iterator cbegin() const noexcept {
        return entries_.cbegin();
    }
    const_iterator cend() const noexcept {
        return entries_.cend();
    }

    /// Appends a header value without altering existing entries for the same key.
    void append(std::string name, std::string value) {
        entries_.emplace_back(std::move(name), std::move(value));
    }

    /// Replaces existing values for \p name with \p value, preserving the
    /// original casing provided by the caller. If the header was absent, the
    /// entry is appended.
    void set(std::string name, std::string value) {
        auto const lower = to_lower(name);
        auto first_match = entries_.end();
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (to_lower(it->first) == lower) {
                if (first_match == entries_.end()) {
                    first_match = it;
                    ++it;
                } else {
                    it = entries_.erase(it);
                }
            } else {
                ++it;
            }
        }
        if (first_match == entries_.end()) {
            entries_.emplace_back(std::move(name), std::move(value));
        } else {
            first_match->first = std::move(name);
            first_match->second = std::move(value);
        }
    }

    /// Provides a map-like emplace interface for callers expecting unordered_map semantics.
    template <typename K, typename V> std::pair<iterator, bool> emplace(K&& name, V&& value) {
        auto const lower = to_lower(name);
        auto it = std::find_if(entries_.begin(), entries_.end(), [&](value_type const& entry) {
            return to_lower(entry.first) == lower;
        });
        if (it != entries_.end()) {
            return {it, false};
        }
        entries_.emplace_back(std::forward<K>(name), std::forward<V>(value));
        return {std::prev(entries_.end()), true};
    }

    /// Returns the first value associated with \p name and inserts an empty
    /// string if the header is absent.
    std::string& operator[](std::string name) {
        auto it = find(name);
        if (it != entries_.end()) {
            return it->second;
        }
        entries_.emplace_back(std::move(name), std::string{});
        return entries_.back().second;
    }

    [[nodiscard]] std::string const& at(std::string_view name) const {
        auto it = find(name);
        if (it == entries_.end()) {
            throw HeaderNotFoundException(std::string{name});
        }
        return it->second;
    }

    [[nodiscard]] std::string& at(std::string_view name) {
        auto it = find(name);
        if (it == entries_.end()) {
            throw HeaderNotFoundException(std::string{name});
        }
        return it->second;
    }

    [[nodiscard]] iterator find(std::string_view name) {
        auto const lower = to_lower(name);
        return std::find_if(entries_.begin(), entries_.end(), [&](value_type const& entry) {
            return to_lower(entry.first) == lower;
        });
    }

    [[nodiscard]] const_iterator find(std::string_view name) const {
        auto const lower = to_lower(name);
        return std::find_if(entries_.cbegin(), entries_.cend(), [&](value_type const& entry) {
            return to_lower(entry.first) == lower;
        });
    }

    [[nodiscard]] std::size_t count(std::string_view name) const {
        auto const lower = to_lower(name);
        return static_cast<std::size_t>(std::count_if(entries_.cbegin(), entries_.cend(), [&](value_type const& entry) {
            return to_lower(entry.first) == lower;
        }));
    }

    /// Erases all occurrences of \p name and returns the number of removed entries.
    std::size_t erase(std::string_view name) {
        auto const lower = to_lower(name);
        auto const original_size = entries_.size();
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                      [&](value_type const& entry) {
                                          return to_lower(entry.first) == lower;
                                      }),
                       entries_.end());
        return original_size - entries_.size();
    }

    [[nodiscard]] std::optional<std::string> get(std::string_view name) const {
        auto it = find(name);
        if (it == entries_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    [[nodiscard]] std::vector<std::string> get_all(std::string_view name) const {
        std::vector<std::string> values;
        auto const lower = to_lower(name);
        for (auto const& [key, value] : entries_) {
            if (to_lower(key) == lower) {
                values.push_back(value);
            }
        }
        return values;
    }

  private:
    static std::string to_lower(std::string_view input) {
        std::string lower;
        lower.reserve(input.size());
        for (unsigned char c : input) {
            lower.push_back(static_cast<char>(std::tolower(c)));
        }
        return lower;
    }

    container_type entries_{};
};

} // namespace alpaca

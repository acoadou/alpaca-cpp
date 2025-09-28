#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "alpaca/Json.hpp"

namespace alpaca {

class Money {
  public:
    static constexpr std::int64_t kScale = 1'000'000; // micro-units

    Money() = default;

    static Money from_raw(std::int64_t micro_units) {
        Money value;
        value.micro_units_ = micro_units;
        return value;
    }

    explicit Money(std::int64_t dollars, std::int64_t fractional) {
        if (fractional >= kScale || fractional <= -kScale) {
            throw std::invalid_argument("Fractional component out of range");
        }
        micro_units_ = dollars * kScale + fractional;
    }

    Money(double value) {
        micro_units_ = static_cast<std::int64_t>(std::llround(value * static_cast<double>(kScale)));
    }

    explicit Money(std::string_view text) {
        micro_units_ = parse(text);
    }

    [[nodiscard]] std::int64_t raw() const {
        return micro_units_;
    }

    [[nodiscard]] double to_double() const {
        return static_cast<double>(micro_units_) / static_cast<double>(kScale);
    }

    [[nodiscard]] std::string to_string(std::size_t min_fractional_digits = 2) const {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(static_cast<int>(std::max<std::size_t>(min_fractional_digits, 2)))
               << to_double();
        return stream.str();
    }

    Money& operator+=(Money const& other) {
        micro_units_ += other.micro_units_;
        return *this;
    }

    Money& operator-=(Money const& other) {
        micro_units_ -= other.micro_units_;
        return *this;
    }

    Money& operator*=(std::int64_t factor) {
        micro_units_ *= factor;
        return *this;
    }

    friend Money operator+(Money lhs, Money const& rhs) {
        lhs += rhs;
        return lhs;
    }

    friend Money operator-(Money lhs, Money const& rhs) {
        lhs -= rhs;
        return lhs;
    }

    friend Money operator*(Money lhs, std::int64_t factor) {
        lhs *= factor;
        return lhs;
    }

    friend Money operator*(std::int64_t factor, Money rhs) {
        rhs *= factor;
        return rhs;
    }

    friend bool operator==(Money const& lhs, Money const& rhs) {
        return lhs.micro_units_ == rhs.micro_units_;
    }

    friend bool operator!=(Money const& lhs, Money const& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(Money const& lhs, Money const& rhs) {
        return lhs.micro_units_ < rhs.micro_units_;
    }

    friend bool operator<=(Money const& lhs, Money const& rhs) {
        return lhs.micro_units_ <= rhs.micro_units_;
    }

    friend bool operator>(Money const& lhs, Money const& rhs) {
        return lhs.micro_units_ > rhs.micro_units_;
    }

    friend bool operator>=(Money const& lhs, Money const& rhs) {
        return lhs.micro_units_ >= rhs.micro_units_;
    }

    friend std::ostream& operator<<(std::ostream& os, Money const& value) {
        os << std::fixed << std::setprecision(2) << value.to_double();
        return os;
    }

  private:
    static std::int64_t parse(std::string_view text) {
        if (text.empty()) {
            return 0;
        }

        bool negative = false;
        std::size_t index = 0;
        if (text[index] == '+') {
            ++index;
        } else if (text[index] == '-') {
            negative = true;
            ++index;
        }

        std::int64_t integer_part = 0;
        for (; index < text.size() && std::isdigit(static_cast<unsigned char>(text[index])); ++index) {
            integer_part = integer_part * 10 + (text[index] - '0');
        }

        std::int64_t fractional_part = 0;
        std::int64_t scale = kScale / 10;
        if (index < text.size() && text[index] == '.') {
            ++index;
            for (; index < text.size() && std::isdigit(static_cast<unsigned char>(text[index])) && scale > 0; ++index) {
                fractional_part += static_cast<std::int64_t>(text[index] - '0') * scale;
                scale /= 10;
            }
        }

        std::int64_t result = integer_part * kScale + fractional_part;
        return negative ? -result : result;
    }

    std::int64_t micro_units_{0};
};

inline void to_json(Json& j, Money const& value) {
    j = value.to_double();
}

inline void from_json(Json const& j, Money& value) {
    if (j.is_null()) {
        value = Money{};
        return;
    }
    if (j.is_number_float()) {
        value = Money{j.get<double>()};
        return;
    }
    if (j.is_number_integer()) {
        value = Money{static_cast<double>(j.get<std::int64_t>())};
        return;
    }
    if (j.is_string()) {
        value = Money{j.get<std::string>()};
        return;
    }
    throw std::invalid_argument("Unsupported JSON type for Money");
}

} // namespace alpaca

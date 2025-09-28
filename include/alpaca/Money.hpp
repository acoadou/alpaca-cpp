#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

#include "alpaca/Exceptions.hpp"
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
            throw InvalidArgumentException("fractional", "Fractional component out of range");
        }
        micro_units_ = dollars * kScale + fractional;
    }

    Money(double value) {
        if (!std::isfinite(value)) {
            throw InvalidArgumentException("value", "Money cannot be constructed from non-finite doubles");
        }

        long double const scaled = static_cast<long double>(value) * static_cast<long double>(kScale);
        long double const max_micro = static_cast<long double>(std::numeric_limits<std::int64_t>::max());
        long double const min_micro = static_cast<long double>(std::numeric_limits<std::int64_t>::min());
        if (scaled >= max_micro + 0.5L || scaled <= min_micro - 0.5L) {
            throw InvalidArgumentException("value", "Money double value exceeds representable range");
        }

        micro_units_ = static_cast<std::int64_t>(std::llround(scaled));
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
        auto const trim = [](std::string_view value) {
            auto const is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
            while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
                value.remove_prefix(1);
            }
            while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
                value.remove_suffix(1);
            }
            return value;
        };

        std::string_view const trimmed = trim(text);
        if (trimmed.empty()) {
            return 0;
        }

        bool negative = false;
        std::size_t index = 0;
        if (trimmed[index] == '+') {
            ++index;
        } else if (trimmed[index] == '-') {
            negative = true;
            ++index;
        }

        constexpr std::uint64_t kMaxPositive = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        constexpr std::uint64_t kMaxNegative = kMaxPositive + 1ULL;
        std::uint64_t const max_micro_units = negative ? kMaxNegative : kMaxPositive;
        std::uint64_t const max_integer_part = max_micro_units / static_cast<std::uint64_t>(kScale);

        std::uint64_t integer_part = 0;
        bool saw_integer_digit = false;
        for (; index < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[index])); ++index) {
            saw_integer_digit = true;
            integer_part = integer_part * 10 + static_cast<std::uint64_t>(trimmed[index] - '0');
            if (integer_part > max_integer_part) {
                throw InvalidArgumentException("text", "Money integer component exceeds representable range");
            }
        }

        std::uint64_t fractional_part = 0;
        std::uint64_t scale = static_cast<std::uint64_t>(kScale / 10);
        bool saw_fractional_digit = false;
        if (index < trimmed.size() && trimmed[index] == '.') {
            ++index;
            for (; index < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[index])) && scale > 0; ++index) {
                saw_fractional_digit = true;
                fractional_part += static_cast<std::uint64_t>(trimmed[index] - '0') * scale;
                scale /= 10;
            }
            if (!saw_fractional_digit) {
                throw InvalidArgumentException("text", "Money fractional component missing digits");
            }
            if (scale == 0 && index < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[index]))) {
                throw InvalidArgumentException("text", "Money supports up to six fractional digits");
            }
        }

        bool const saw_any_digit = saw_integer_digit || saw_fractional_digit;
        if (!saw_any_digit) {
            throw InvalidArgumentException("text", "Money text must contain digits");
        }

        std::uint64_t magnitude = integer_part * static_cast<std::uint64_t>(kScale);
        std::uint64_t const fractional_limit = max_micro_units - magnitude;
        if (fractional_part > fractional_limit) {
            throw InvalidArgumentException("text", "Money fractional component exceeds representable range");
        }
        magnitude += fractional_part;

        if (index != trimmed.size()) {
            throw InvalidArgumentException("text", "Unexpected trailing characters in Money text");
        }

        if (negative) {
            if (magnitude == kMaxNegative) {
                return std::numeric_limits<std::int64_t>::min();
            }
            return -static_cast<std::int64_t>(magnitude);
        }
        return static_cast<std::int64_t>(magnitude);
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
    throw InvalidArgumentException("value", "Unsupported JSON type for Money");
}

} // namespace alpaca

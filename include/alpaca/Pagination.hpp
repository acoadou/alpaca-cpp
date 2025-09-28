#pragma once

#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "alpaca/Exceptions.hpp"

namespace alpaca {

/// Single-pass range adaptor that iterates over paginated Alpaca endpoints while handling
/// rate limiting via Retry-After headers.
template <typename Request, typename Page, typename Value> class PaginatedVectorRange {
  public:
    using FetchPage = std::function<Page(Request const&)>;
    using Extractor = std::function<std::vector<Value> const&(Page const&)>;
    using CursorAccessor = std::function<std::optional<std::string>(Page const&)>;
    using CursorMutator = std::function<void(Request&, std::optional<std::string> const&)>;

    PaginatedVectorRange(Request request, FetchPage fetch_page, Extractor extractor, CursorAccessor get_cursor,
                         CursorMutator set_cursor)
      : request_(std::move(request)), fetch_(std::move(fetch_page)), extractor_(std::move(extractor)),
        cursor_getter_(std::move(get_cursor)), cursor_setter_(std::move(set_cursor)) {
    }

    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Value;
        using difference_type = std::ptrdiff_t;
        using pointer = Value const*;
        using reference = Value const&;

        iterator() = default;

        reference operator*() const {
            return (*range_->current_items_)[range_->index_];
        }

        pointer operator->() const {
            return &(*range_->current_items_)[range_->index_];
        }

        iterator& operator++() {
            range_->advance();
            if (!range_->current_items_) {
                end_ = true;
            }
            return *this;
        }

        iterator operator++(int) {
            iterator tmp(*this);
            ++(*this);
            return tmp;
        }

        friend bool operator==(iterator const& lhs, iterator const& rhs) {
            return lhs.range_ == rhs.range_ && lhs.end_ == rhs.end_;
        }

        friend bool operator!=(iterator const& lhs, iterator const& rhs) {
            return !(lhs == rhs);
        }

      private:
        friend class PaginatedVectorRange;

        iterator(PaginatedVectorRange* range, bool end) : range_(range), end_(end) {
        }

        PaginatedVectorRange* range_{nullptr};
        bool end_{true};
    };

    iterator begin() {
        ensure_started();
        if (!current_items_) {
            return iterator(this, true);
        }
        return iterator(this, false);
    }

    iterator end() {
        return iterator(this, true);
    }

    /// Access to the request being replayed across pages.
    Request const& request() const noexcept {
        return request_;
    }

  private:
    void ensure_started() {
        if (!started_) {
            fetch_page();
            started_ = true;
        }
    }

    void advance() {
        if (!current_items_) {
            return;
        }
        ++index_;
        if (index_ < current_items_->size()) {
            return;
        }
        if (finished_) {
            current_items_ = nullptr;
            return;
        }
        fetch_page();
    }

    void fetch_page() {
        while (true) {
            try {
                current_page_ = fetch_(request_);
            } catch (Exception const& ex) {
                if (auto retry = ex.retry_after()) {
                    std::this_thread::sleep_for(*retry);
                    continue;
                }
                throw;
            }

            if (!current_page_.has_value()) {
                current_items_ = nullptr;
                finished_ = true;
                return;
            }

            current_items_ = &extractor_(*current_page_);
            index_ = 0;

            auto next = cursor_getter_(*current_page_);
            if (next.has_value()) {
                cursor_setter_(request_, next);
                finished_ = false;
            } else {
                finished_ = true;
            }

            if (!current_items_->empty()) {
                return;
            }

            if (finished_) {
                current_items_ = nullptr;
                return;
            }
        }
    }

    Request request_;
    FetchPage fetch_;
    Extractor extractor_;
    CursorAccessor cursor_getter_;
    CursorMutator cursor_setter_;

    std::optional<Page> current_page_{};
    std::vector<Value> const* current_items_{nullptr};
    std::size_t index_{0};
    bool finished_{false};
    bool started_{false};
};

} // namespace alpaca

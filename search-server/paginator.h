#pragma once

#include <utility>
#include <vector>
#include <ostream>

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, int page_size) {
        auto it_first = begin;
        auto it_second = begin;
        std::advance(it_second, page_size);
        for(int i = 0; i < distance(begin, end) / page_size; ++i) {
            pages_.push_back({it_first, it_second});
            std::advance(it_first, page_size);
            std::advance(it_second, page_size);
        }
        auto it_end = end;
        if(distance(begin, end) % page_size) {
            if(it_first != end) {
                if(it_first != it_end) {
                    pages_.push_back({it_first, it_end});
                }
                else {
                    it_end = it_first;
                    std::advance(it_end, 1);
                    pages_.push_back({it_first, it_end});
                }
            }
        }
    }
    
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    
    class IteratorRange {
    public:
        IteratorRange(Iterator begin, Iterator end)
                : iterator_range_({begin, end}){
        }
        
        Iterator begin() const {
            return iterator_range_.first;
        }
        
        Iterator end() const {
            return iterator_range_.second;
        }
        
        int size() const {
            return std::distance(iterator_range_.first, iterator_range_.second);
        }
        
        friend std::ostream& operator<<(std::ostream& output,  const IteratorRange& iteratorRange) {
            for(auto i = iteratorRange.begin(); i != iteratorRange.end(); ++i) {
                output << *i;
            }
            return output;
        }
    private:
        std::pair<Iterator, Iterator> iterator_range_;
    };
private:
    std::vector<IteratorRange> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

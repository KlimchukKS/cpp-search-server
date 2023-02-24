#pragma once

#include <map>
#include <vector>
#include <mutex>
#include <string>


template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    struct Bucket {
        std::mutex mu;
        std::map<Key, Value> ma;
    };

    explicit ConcurrentMap(const std::size_t bucket_count)
            : buckets_(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto index = static_cast<std::uint64_t>(key) % buckets_.size();
        return {std::lock_guard(buckets_[index].mu), buckets_[index].ma[key]};
    }

    void Erase(const Key& key) {
        auto index = static_cast<std::uint64_t>(key) % buckets_.size();
        std::lock_guard<std::mutex> guard(buckets_[index].mu);
        buckets_[index].ma.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> ans;

        for (auto& [mut, ma] : buckets_) {
            std::lock_guard guard(mut);
            for (auto [k, v] : ma) {
                ans[k] = v;
            }
        }
        return ans;
    }

private:
    std::vector<Bucket> buckets_;
};

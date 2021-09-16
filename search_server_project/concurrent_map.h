#pragma once

#include <map>
#include <string>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        explicit Access(std::map<Key, Value>& map, const Key& key, std::mutex &value_mutex)
        : guard(value_mutex)
        , map_(map)
        , ref_to_value(map_[key])
          {}
        
        std::lock_guard<std::mutex> guard;                
        std::map<Key, Value>& map_;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
    : bucket_count_(bucket_count)
    , maps_(bucket_count)
    , v_mutex_(bucket_count){}

    Access operator[](const Key& key){
        uint64_t bucket = key % bucket_count_;       
        return Access(maps_[bucket], key, v_mutex_[bucket]);
    }

    std::map<Key, Value> BuildOrdinaryMap(){
        std::map<Key, Value> result;
        for(size_t i = 0; i < maps_.size(); ++i){
            for(auto it = maps_[i].begin(); it != maps_[i].end(); ++it){
                Access access(maps_[i], it->first, v_mutex_[i]);               
                result[it->first] = access.ref_to_value;
            }
        } 
        return result;
    }

private:
    size_t bucket_count_;
    std::vector<std::map<Key, Value>> maps_;
    std::vector<std::mutex> v_mutex_;
};

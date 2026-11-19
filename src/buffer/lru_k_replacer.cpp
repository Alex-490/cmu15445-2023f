//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}


auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
    std::lock_guard<std::mutex> lock(latch_);

    std::vector<frame_id_t> vet1,vet2;
    bool flag=false;

    for(auto it:node_store_){
        auto id=it.first;
        auto node=it.second;//参数用node还是LRUKNnode&？
        if(!node.isEvictable()) continue;

        //存在访问次数不足k的那就只需要考虑这些
        if(node.size()<k_){
            vet1.emplace_back(id);
            flag=true;
            continue;
        }

        //有至少一个vet1元素，就不考虑vet2元素
        if(flag)    continue;
        vet2.emplace_back(id);
    }

    //找不足k次里面最早被访问的
    if(flag){
        //不需要再判断vet1是否非空因为flag为true肯定非空
        size_t min=current_timestamp_;
        frame_id_t target=-1;
        for(auto id:vet1){
            auto t=node_store_[id].earliest();
            if(t<min){
                min=t;
                target=id;
            }
        }
        //此时已经找到了
        node_store_.erase(target);
        *frame_id=target;
        --curr_size_;
        return true;
    }else{
        //如果vet1为空，vet2也为空，那么返回false
        if(vet2.empty())    return false;
        //否则就从backward k-distance里面找
        size_t max=0;
        frame_id_t target=-1;
        for(auto id:vet2){
            //使用迭代器？
            size_t earliest=node_store_[id].earliest();
            if(current_timestamp_-earliest>max){
                max=current_timestamp_-earliest;
                target=id;
            }
        }
        
        //此时已经找到了
        node_store_.erase(target);
        *frame_id=target;
        --curr_size_;
        return true;
    }

    //都没就没得evict，返回false
    return false;
}

//记录历史
void LRUKReplacer::RecordAccess(frame_id_t frame_id,AccessType access_type) {
    std::lock_guard<std::mutex> lock(latch_);
    BUSTUB_ASSERT(frame_id<replacer_size_,"invalid frame id");
    
    auto it=node_store_.find(frame_id);
    //没有就添加一个新的
    if(it==node_store_.end()){
        //这里需要++么？时间戳到底怎么设计的？
        node_store_[frame_id]=LRUKNode(k_,current_timestamp_++);//怪不得需要写构造函数和默认构造函数
    }else{
        LRUKNode& node=node_store_[frame_id];
        node.access(current_timestamp_++);
    }
}

//* If frame id is invalid, throw an exception or abort the process.  怎么abort()?
void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    std::lock_guard<std::mutex> lock(latch_);
    BUSTUB_ASSERT(frame_id<replacer_size_,"invalid frame id");
    //throw std::exception();

    auto it=node_store_.find(frame_id);
    if(it==node_store_.end())    return;

    LRUKNode& node=it->second;
    if(set_evictable){
        if(!node.isEvictable()) ++curr_size_;
    }else{
        if(node.isEvictable()) --curr_size_;
    }

    node.setEvictable(set_evictable);
}

//从map上erase指定lruknode，因为key是匿名node而不是指针形式
void LRUKReplacer::Remove(frame_id_t frame_id) {
    std::lock_guard<std::mutex> lock(latch_);
    
    auto it=node_store_.find(frame_id);
    if(it==node_store_.end())    return;
    LRUKNode& node=it->second;
    if(!node.isEvictable()){
        //抛出异常或者终止程序
        throw std::logic_error("can not remove a unevictable frame");
    }
    
    //remove
    node_store_.erase(it);
    --curr_size_;
}

auto LRUKReplacer::Size() -> size_t {
    std::lock_guard<std::mutex> lock(latch_);
    return curr_size_;
}

}  // namespace bustub

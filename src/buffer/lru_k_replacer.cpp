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
    std::scoped_lock<std::mutex> lock(latch_);

    bool flag=false;
    LRUKNode* target_node=nullptr;

    size_t mx_dis=0;
    size_t mx_dis_=0;
    bool success=false;

    for(auto &it:node_store_){
        auto &node=it.second;
        
        if(!node.is_evictable_) continue;
        
        success=true;

        if(node.history_.empty()){
            target_node=&node;
            *frame_id=it.first;
            break;
        }

        if(node.history_.size()==k_&&!flag){
            auto cur_dis=current_timestamp_-node.GetBackKStamp();
            if(cur_dis>mx_dis){
                mx_dis=cur_dis;
                target_node=&node;
                *frame_id=it.first;
            }
        }
        else if(node.history_.size()<k_&&flag){
            flag=true;
            auto cur_dis_=current_timestamp_-node.GetLatestStamp();
            if(cur_dis_>mx_dis_){
                mx_dis_=cur_dis_;
                target_node=&node;
                *frame_id=it.first;
            }
        }
    }

    if(success){
        curr_size_--;
        target_node->history_.clear();
        node_store_.erase(*frame_id);
    }

    return success; 
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id,AccessType access_type) {
    std::scoped_lock<std::mutex> lock(latch_);

    auto it=node_store_.find(frame_id);

    if(it==node_store_.end()){
        auto new_node=std::make_shared<LRUKNode>();
        if(access_type!=AccessType::Scan){
            new_node->history_.push_back(current_timestamp_++);
        }
        node_store_.insert({frame_id,*new_node});
    }else{
        auto &node=it->second;
        if(access_type!=AccessType::Scan){
            if(node.history_.size()==k_) node.history_.pop_front();
            node.history_.push_back(current_timestamp_++);
        }
    }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    std::scoped_lock<std::mutex> lock(latch_);

    auto it=node_store_.find(frame_id);
    if(it==node_store_.end()){
        node_store_.insert({frame_id,LRUKNode()});
        it=node_store_.find(frame_id);
    }
    
    auto& node=it->second;
    if(set_evictable&&!node.is_evictable_){
        node.is_evictable_=true;
        curr_size_++;
    }
    if(!set_evictable&&node.is_evictable_){
        node.is_evictable_=false;
        curr_size_--;
    }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
    std::scoped_lock<std::mutex> lock(latch_);
    auto it=node_store_.find(frame_id);

    if(it==node_store_.end())   return;

    auto& node=it->second;

    node.history_.clear();
    node_store_.erase(frame_id);
    curr_size_--;
}

auto LRUKReplacer::Size() -> size_t {
    return curr_size_;
}

}  // namespace bustub

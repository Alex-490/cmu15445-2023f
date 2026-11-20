#include "storage/page/page_guard.h"
#include "buffer/buffer_pool_manager.h"

namespace bustub {

//移动应该不需要pin
BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept 
    :bpm_(that.bpm_),page_(that.page_),is_dirty_(that.is_dirty_){
    that.bpm_=nullptr;
    that.page_=nullptr;
    that.is_dirty_=false;
}

//手动释放资源
void BasicPageGuard::Drop() {
    if(bpm_!=nullptr&&page_!=nullptr){
        bpm_->UnpinPage(page_->GetPageId(),is_dirty_);
    }
    bpm_=nullptr;
    page_=nullptr;
    is_dirty_=false;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
    //if(*this==that) return *this;
    if(this==&that) return *this;
    
    //忘了释放原有资源
    Drop();

    bpm_=that.bpm_;
    page_=that.page_;
    is_dirty_=that.is_dirty_;
    
    that.bpm_=nullptr;
    that.page_=nullptr;
    that.is_dirty_=false;

    return *this;
}

BasicPageGuard::~BasicPageGuard(){
    Drop();
};  // NOLINT

auto BasicPageGuard::UpgradeRead() -> ReadPageGuard{
    //不能用构造函数会丢失is_dirty_的状态
    //ReadPageGuard rpg(bpm_,page_);
    if(page_==nullptr){
        return {};
    }
    
    page_->RLatch();
    return ReadPageGuard(std::move(*this));
}

auto BasicPageGuard::UpgradeWrite() -> WritePageGuard{
    //不能用构造函数会丢失is_dirty_的状态
    //WritePageGuard wpg(bpm_,page_);
    if(page_==nullptr){
        return {};
    }

    page_->WLatch();
    return WritePageGuard(std::move(*this));
}

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept 
    :guard_(std::move(that.guard_)){
}

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
    if(this==&that) return *this;

    //忘了释放资源
    Drop();

    guard_=std::move(that.guard_);
    return *this;
}

void ReadPageGuard::Drop() {
    //检查条件防止二次Drop()
    if(guard_.page_!=nullptr){
        guard_.page_->RUnlatch();
    }
    guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() {
    Drop();
}  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept
    :guard_(std::move(that.guard_)){
}

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
    if(this==&that)  return *this;
    
    //忘了释放资源
    Drop();
    
    guard_=std::move(that.guard_);
}

void WritePageGuard::Drop() {
    //防止二次Drop()
    if(guard_.page_!=nullptr){
        guard_.page_->WUnlatch();
    }
    guard_.Drop();
}

WritePageGuard::~WritePageGuard() {
    Drop();
}  // NOLINT

}  // namespace bustub

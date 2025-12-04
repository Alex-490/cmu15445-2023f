#include "storage/page/page_guard.h"
#include "buffer/buffer_pool_manager.h"

namespace bustub {

BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
    Drop();
    is_dirty_=that.is_dirty_;
    bpm_=that.bpm_;
    page_=that.page_;
    that.is_dirty_=false;
    that.bpm_=nullptr;
    that.page_=nullptr;
}

void BasicPageGuard::Drop() {
    if(page_!=nullptr){
        bpm_->UnpinPage(PageId(),is_dirty_);
    }
    page_=nullptr;
    bpm_=nullptr;
    is_dirty_=false;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
    Drop();
    is_dirty_=that.is_dirty_;
    bpm_=that.bpm_;
    page_=that.page_;
    that.is_dirty_=false;
    that.bpm_=nullptr;
    that.page_=nullptr;
    return *this;
}

BasicPageGuard::~BasicPageGuard(){
    Drop();
};  // NOLINT

auto BasicPageGuard::UpgradeRead() -> ReadPageGuard{
    page_->RLatch();
    ReadPageGuard ret(bpm_,page_);

    bpm_=nullptr;
    page_=nullptr;
    return ret;
}

auto BasicPageGuard::UpgradeWrite() -> WritePageGuard{
    page_->WLatch();
    WritePageGuard ret(bpm_,page_);

    bpm_=nullptr;
    page_=nullptr;
    return ret;
}

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept = default;

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
    Drop();
    guard_=std::move(that.guard_);
    return *this;
}

void ReadPageGuard::Drop() {
    if(guard_.page_!=nullptr){
        guard_.page_->RUnlatch();
    }
    
    guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() {
    Drop();
}  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept = default;

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
    Drop();
    guard_=std::move(that.guard_);
    
    return *this;
}

void WritePageGuard::Drop() {
    if(guard_.page_!=nullptr){
        guard_.page_->WUnlatch();
    }

    guard_.Drop();
}

WritePageGuard::~WritePageGuard() {
    Drop();
}  // NOLINT

}  // namespace bustub

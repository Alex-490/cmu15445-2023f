//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan) : 
    AbstractExecutor(exec_ctx) ,plan_(plan){}

void SeqScanExecutor::Init() {
    //通过plan_获取里面table_oid_t指向的那张表的TableHeap
    tableInfo_=exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
    tableHeap_=tableInfo_->table_.get();

    //获得tableheap的迭代器
    it_=std::make_unique<TableIterator>(tableHeap_->MakeEagerIterator());
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    while(!it_->IsEnd()){
        auto [meta,newTuple]=it_->GetTuple();
        if(!meta.is_deleted_){
            *tuple=newTuple;
            *rid=it_->GetRID();
            ++(*it_);
            return true;
        }
        ++(*it_);
    }
    return false;
}

}  // namespace bustub

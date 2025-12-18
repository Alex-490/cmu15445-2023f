//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/insert_executor.h"

namespace bustub {

InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) ,plan_(plan),child_executor_(std::move(child_executor)){}

void InsertExecutor::Init() {
    child_executor_->Init();
    table_info_=exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
    index_infos_=exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
}

auto InsertExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(has_done_) return false;

    int count=0;
    Tuple child_tuple;
    RID child_rid;

    while(child_executor_->Next(&child_tuple,&child_rid)){
        TupleMeta meta_tmp{0,false};
        auto new_rid=table_info_->table_->InsertTuple(meta_tmp,child_tuple);

        BUSTUB_ASSERT(new_rid.has_value(),"Insert failed");
        for(auto *index_info:index_infos_){
        index_info->index_->InsertEntry(
            child_tuple.KeyFromTuple(table_info_->schema_,index_info->key_schema_,index_info->index_->GetKeyAttrs()),
            new_rid.value(),
            exec_ctx_->GetTransaction());
        }
        ++count;
    }
    
    std::vector<Value> ret;
    ret.emplace_back(INTEGER,count);
    *tuple=Tuple(ret,&GetOutputSchema());
    *rid=RID();

    has_done_=true;
    return true;
}

}  // namespace bustub

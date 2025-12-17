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

auto InsertExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
    if(is_finished) return false;
    is_finished=true;

    Tuple child_tuple;
    RID child_rid;

    auto status=child_executor_->Next(&child_tuple,&child_rid);
    while(status){
        TupleMeta meta_tmp;
        meta_tmp.is_deleted_=false;
        meta_tmp.ts_=0;
        auto rid_tmp=table_info_->table_->InsertTuple(meta_tmp,child_tuple);

        if(rid_tmp.has_value()){
            for(auto *index_info:index_infos_){
            index_info->index_->InsertEntry(
                child_tuple.KeyFromTuple(table_info_->schema_,index_info->key_schema_,index_info->index_->GetKeyAttrs()),
                rid_tmp.value(),
                exec_ctx_->GetTransaction());
            }

            status=child_executor_->Next(&child_tuple,&child_rid);
            ++count_;
        }
    }
    
    std::vector<Value> ret;
    ret.emplace_back(INTEGER,count_);
    *tuple=Tuple(ret,&GetOutputSchema());

    return true;
}

}  // namespace bustub

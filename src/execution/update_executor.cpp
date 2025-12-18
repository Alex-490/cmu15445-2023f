//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>

#include "execution/executors/update_executor.h"

namespace bustub {

UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) ,plan_(plan),child_executor_(std::move(child_executor)){
  // As of Fall 2022, you DON'T need to implement update executor to have perfect score in project 3 / project 4.
}

void UpdateExecutor::Init() {
  child_executor_->Init();
  table_info_=exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
  index_infos_=exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);  
  //throw NotImplementedException("UpdateExecutor is not implemented");
}

auto UpdateExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if(has_done_)  return false;
  
  int count=0;
  Tuple old_tuple;
  RID old_rid;

  while(child_executor_->Next(&old_tuple,&old_rid)){
    auto old_meta=table_info_->table_->GetTupleMeta(old_rid);
    old_meta.is_deleted_=true;
    table_info_->table_->UpdateTupleMeta(old_meta,old_rid);
    
    std::vector<Value> values;
    for(auto &expr:plan_->target_expressions_){
      values.emplace_back(expr->Evaluate(&old_tuple,child_executor_->GetOutputSchema()));
    }

    Tuple new_tuple=Tuple(values,&table_info_->schema_);
    TupleMeta new_meta{0,false};
    auto new_rid=table_info_->table_->InsertTuple(new_meta,new_tuple);
    BUSTUB_ASSERT(new_rid.has_value(),"Update failed");

    for(auto* index_info:index_infos_){
      index_info->index_->DeleteEntry(
        old_tuple.KeyFromTuple(
          table_info_->schema_,
          index_info->key_schema_,
          index_info->index_->GetKeyAttrs()),
        old_rid,
        exec_ctx_->GetTransaction());

      index_info->index_->InsertEntry(
        new_tuple.KeyFromTuple(
            table_info_->schema_,
            index_info->key_schema_,
            index_info->index_->GetKeyAttrs()),
        new_rid.value(),
        exec_ctx_->GetTransaction());
    }

    ++count;
  }

  std::vector<Value> ret;
  ret.emplace_back(INTEGER,count);
  *tuple=Tuple(ret,&GetOutputSchema());

  has_done_=true;
  return true;
}

}  // namespace bustub

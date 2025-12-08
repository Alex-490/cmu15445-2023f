//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "execution/executors/delete_executor.h"

namespace bustub {

DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) ,plan_(plan),child_executor_(std::move(child_executor)){}

void DeleteExecutor::Init() {
    count_=0;
    child_executor_->Init();
    table_info_=exec_ctx_->GetCatalog()->GetTable(plan_->table_oid_);
    index_infos_=exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
    //throw NotImplementedException("DeleteExecutor is not implemented");
}

auto DeleteExecutor::Next([[maybe_unused]] Tuple *tuple, RID *rid) -> bool {
    if(is_finished_)    return false;

    Tuple tuple_tmp;
    RID rid_tmp;
    auto status=child_executor_->Next(&tuple_tmp,&rid_tmp);
    if(status) is_finished_=true;

    while(status){
        auto meta=table_info_->table_->GetTupleMeta(rid_tmp);
        meta.is_deleted_=true;
        table_info_->table_->UpdateTupleMeta(meta,rid_tmp);

        for(auto *index_info:index_infos_){
            index_info->index_->DeleteEntry(
                tuple_tmp.KeyFromTuple(table_info_->schema_,index_info->key_schema_,index_info->index_->GetKeyAttrs()),
                rid_tmp,exec_ctx_->GetTransaction());
        }

        status=child_executor_->Next(&tuple_tmp,&rid_tmp);
        count_++;
    }
    
    std::vector<Value> ret;
    ret.emplace_back(INTEGER,count_);
    *tuple=tuple_tmp;
    return true;
}

}  // namespace bustub

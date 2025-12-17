//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-19, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "execution/executors/index_scan_executor.h"

namespace bustub {
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx) ,plan_(plan){}

void IndexScanExecutor::Init() {
    h_table_=dynamic_cast<HashTableIndexForTwoIntegerColumn *>(
        exec_ctx_->GetCatalog()->GetIndex(plan_->GetIndexOid())->index_.get());
}

auto IndexScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(is_finished) return false;
    is_finished=true;

    std::vector<RID> result_tmp;
    std::vector<Value> value_tmp;
    value_tmp.emplace_back(plan_->pred_key_->val_);
    Schema schema=exec_ctx_->GetCatalog()->GetIndex(plan_->GetIndexOid())->key_schema_;

    Tuple tuple_tmp=Tuple(value_tmp,&schema);

    h_table_->ScanKey(tuple_tmp,&result_tmp,exec_ctx_->GetTransaction());

    if(result_tmp.empty()) return false;

    auto rid_tmp=result_tmp[0];

    auto [meta_tmp,tuple_tmp]=exec_ctx_->GetCatalog()->GetTable(plan_->table_oid_)->table_->GetTuple(rid_tmp);

    if(meta_tmp.is_deleted_)   return false;

    if(plan_->filter_predicate_){
        auto value=plan_->filter_predicate_->Evaluate(&tuple_tmp,GetOutputSchema());
        if(value.IsNull()||value.GetAs<bool>()){
            return false;
        }
    }

    *tuple=tuple_tmp;
    *rid=tuple_tmp.GetRid();
    return true;
}

}  // namespace bustub

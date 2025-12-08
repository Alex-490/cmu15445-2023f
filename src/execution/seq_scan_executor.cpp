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
    auto table_info=exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
    iter_=std::make_shared<TableIterator>(table_info->table_->MakeIterator());
    //throw NotImplementedException("SeqScanExecutor is not implemented");
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(iter_->IsEnd())   return false;
    
    Tuple tuple_tmp;
    TupleMeta meta_tmp;

    while(!iter_->IsEnd()){
        auto pair_tmp=iter_->GetTuple();
        ++(*iter_);
        meta_tmp=pair_tmp.first;
        if(meta_tmp.is_deleted_)    continue;
        tuple_tmp=pair_tmp.second;

        if(plan_->filter_predicate_){
            auto value=plan_->filter_predicate_->Evaluate(&tuple_tmp,GetOutputSchema());
            if(!value.IsNull()&&value.GetAs<bool>()){
                *tuple=tuple_tmp;
                *rid=tuple_tmp.GetRid();
                return true;
            }
        }else{
            *tuple=tuple_tmp;
            *rid=tuple_tmp.GetRid();
            return true;
        }
    }
    
    return false;
}

}  // namespace bustub

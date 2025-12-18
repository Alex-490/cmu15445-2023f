//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include <memory>
#include <vector>

#include "execution/executors/aggregation_executor.h"

namespace bustub {

AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {}

void AggregationExecutor::Init() {
    aht_->Clear();
    child_executor_->Init();
    Tuple tuple_tmp;
    RID rid_tmp;

    bool has_tuple=false;

    while(child_executor_->Next(&tuple_tmp,&rid_tmp)){
        auto key=MakeAggregateKey(&tuple_tmp);
        auto val=MakeAggregateValue(&tuple_tmp);
        aht_->InsertCombine(key,val);
        has_tuple=true;
    }

    if(!has_tuple&&plan_->group_bys_.empty()){
        aht_->InitAsEmpty();
    }

    aht_iterator_=std::make_unique<SimpleAggregationHashTable::Iterator>(aht_->Begin());
}

auto AggregationExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(*aht_iterator_==aht_->End())  return false;

    std::vector<Value> ret_values;
    ret_values.insert(ret_values.end(),aht_iterator_->Key().group_bys_.begin(),aht_iterator_->Key().group_bys_.end());
    ret_values.insert(ret_values.end(),aht_iterator_->Val().aggregates_.begin(),aht_iterator_->Val().aggregates_.end());

    *tuple=Tuple(ret_values,&GetOutputSchema());
    ++(*aht_iterator_);

    return true;
}

auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_executor_.get(); }

}  // namespace bustub

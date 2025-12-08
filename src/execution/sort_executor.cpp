#include "execution/executors/sort_executor.h"

namespace bustub {

SortExecutor::SortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) ,plan_(plan),child_executor_(std::move(child_executor)){}

void SortExecutor::Init() {
    child_executor_->Init();
    tuples_.clear();

    Tuple tuple_tmp;
    RID rid_tmp;
    auto has_next=child_executor_->Next(&tuple_tmp,&rid_tmp);
    while(has_next){
        tuples_.emplace_back(std::make_pair(tuple_tmp,rid_tmp));
        has_next=child_executor_->Next(&tuple_tmp,&rid_tmp);
    }

    std::sort(tuples_.begin(),tuples_.end(),CompareTuple(&plan_->GetOrderBy(),&child_executor_->GetOutputSchema()));

    iter_=tuples_.begin();
    //throw NotImplementedException("SortExecutor is not implemented");
}

auto SortExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(iter_==tuples_.end())    return false;
    
    *tuple=iter_->first;
    *rid=iter_->second;
    iter_++;
    return true;
}

}  // namespace bustub

#include "execution/executors/topn_executor.h"

namespace bustub {

TopNExecutor::TopNExecutor(ExecutorContext *exec_ctx, const TopNPlanNode *plan,
                           std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) ,plan_(plan),child_executor_(std::move(child_executor)){}

void TopNExecutor::Init() {
    if(!tuples_.empty())    return;
    tuples_.clear();
    child_executor_->Init();
    CompareTuple comp(&plan_->GetOrderBy(),&child_executor_->GetOutputSchema());

    std::priority_queue<std::pair<Tuple,RID>,std::vector<std::pair<Tuple,RID>>,CompareTuple> queue(comp);

    Tuple tuple_tmp;
    RID rid_tmp;

    auto status=child_executor_->Next(&tuple_tmp,&rid_tmp);
    std::pair<Tuple,RID> pair_tmp={tuple_tmp,rid_tmp};

    while(status){
        if(queue.size()<plan_->GetN()){
            queue.push(pair_tmp);
        }else{
            auto top=queue.top();
            if(comp(pair_tmp,top)){
                queue.pop();
                queue.push(pair_tmp);
            }
        }
        status=child_executor_->Next(&tuple_tmp,&rid_tmp);
        pair_tmp={tuple_tmp,rid_tmp};
    }

    unsigned count=0;
    while(queue.size()&&count<plan_->GetN()){
        tuples_.emplace_back(queue.top());
        queue.pop();
        count++;
    }

    iter_=tuples_.rbegin();
}

auto TopNExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(iter_==tuples_.rend())   return false;
    
    *tuple=iter_->first;
    *rid=iter_->second;
    iter_++;
    return true;
}

auto TopNExecutor::GetNumInHeap() -> size_t {return tuples_.size();};

}  // namespace bustub

//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"
#include "type/value_factory.h"

namespace bustub {

HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_child,
                                   std::unique_ptr<AbstractExecutor> &&right_child)
    : AbstractExecutor(exec_ctx) ,plan_(plan),left_child_executor_(std::move(left_child)),
      right_child_executor_(std::move(right_child)){
  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2023 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void HashJoinExecutor::Init() {
  left_child_executor_->Init();
  right_child_executor_->Init();
  ht_.clear();
  candidate_tuples_.clear();

  Tuple tuple_left;
  RID rid_left;

  bool status;
  while(status=right_child_executor_->Next(&tuple_left,&rid_left)){
    HashJoinKey key=MakeHashJoinKey(&tuple_left,plan_->left_key_expressions_,left_child_executor_->GetOutputSchema());
    ht_[key].tuples_.emplace_back(tuple_left);
  }
  
  iter_=ht_.begin();
}

auto HashJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if(candidate_tuples_.size())  return true;

  Tuple tuple_right;
  RID rid_right;

  bool status;
  while(status=right_child_executor_->Next(&tuple_right,&rid_right)){
    HashJoinKey key=MakeHashJoinKey(&tuple_right,plan_->right_key_expressions_,right_child_executor_->GetOutputSchema());
    if(ht_.find(key)!=ht_.end()){
      for(auto &tuple:ht_[key].tuples_){
        std::vector<Value> out_values;
        for(int i=0;i<plan_->GetLeftPlan()->OutputSchema().GetColumns().size();i++){
          out_values.emplace_back(tuple.GetValue(&plan_->GetLeftPlan()->OutputSchema(),i));
        }
        for(int i=0;i<plan_->GetRightPlan()->OutputSchema().GetColumns().size();i++){
          out_values.emplace_back(tuple.GetValue(&plan_->GetRightPlan()->OutputSchema(),i));
        }
        candidate_tuples_.emplace_back(Tuple(out_values,&GetOutputSchema()));
      }
      ht_[key].is_joined_=true;
      *tuple=candidate_tuples_.back();
      candidate_tuples_.pop_back();
      return true;
    }
  }

  while(iter_!=ht_.end()){
    if(iter_->second.is_joined_==false){
      for(auto &tuple:iter_->second.tuples_){
        std::vector<Value> out_values;
        for(int i=0;i<plan_->GetLeftPlan()->OutputSchema().GetColumns().size();i++){
          out_values.emplace_back(tuple.GetValue(&plan_->GetLeftPlan()->OutputSchema(),i));
        }
        for(int i=0;i<plan_->GetRightPlan()->OutputSchema().GetColumns().size();i++){
          out_values.emplace_back(ValueFactory::GetNullValueByType(plan_->GetRightPlan()->OutputSchema().GetColumn(i).GetType()));
        }
        candidate_tuples_.emplace_back(Tuple(out_values,&GetOutputSchema()));
      }
      iter_->second.is_joined_=true;
      *tuple=candidate_tuples_.back();
      candidate_tuples_.pop_back();
      return true;
    }
    iter_++;
  }
}

}  // namespace bustub

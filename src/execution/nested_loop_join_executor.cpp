//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"
#include "binder/table_ref/bound_join_ref.h"
#include "common/exception.h"
#include "type/value_factory.h"

namespace bustub {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx) ,plan_(plan),left_executor_(std::move(left_executor)),
      right_executor_(std::move(right_executor)){
  if (!(plan->GetJoinType() == JoinType::LEFT || plan->GetJoinType() == JoinType::INNER)) {
    // Note for 2023 Fall: You ONLY need to implement left join and inner join.
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void NestedLoopJoinExecutor::Init() {
  left_executor_->Init();
  right_executor_->Init();

  if(!left_executor_->Next(&left_tuple_,&left_rid_)){
    has_done_=true;
  }
}

auto NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if(has_done_) return false;
  Tuple right_tuple;
  RID right_rid;

  while(true){
    while(true){
      if(right_executor_->Next(&right_tuple,&right_rid)){
        break;
      }

      has_right_tuple_=true;
      Value value=plan_->Predicate()->EvaluateJoin(&left_tuple_,plan_->GetLeftPlan()->OutputSchema(),&right_tuple,plan_->GetRightPlan()->OutputSchema());
      if(!value.IsNull()&&value.GetAs<bool>()){
        std::vector<Value> output;
        for(int i=0;i<plan_->GetLeftPlan()->OutputSchema().GetColumns().size();i++){
          output.emplace_back(left_tuple_.GetValue(&plan_->GetLeftPlan()->OutputSchema(),i));
        }
        for(int i=0;i<plan_->GetRightPlan()->OutputSchema().GetColumns().size();i++){
          output.emplace_back(right_tuple.GetValue(&plan_->GetRightPlan()->OutputSchema(),i));
        }
        *tuple=Tuple(output,&GetOutputSchema());
        return true;
      }
    }

    if(plan_->GetJoinType()==JoinType::LEFT&&!has_right_tuple_){
      std::vector<Value> output;
      for(int i=0;i<plan_->GetLeftPlan()->OutputSchema().GetColumns().size();i++){
        output.emplace_back(left_tuple_.GetValue(&plan_->GetLeftPlan()->OutputSchema(),i));
      }
      for(int i=0;i<plan_->GetRightPlan()->OutputSchema().GetColumns().size();i++){
        output.emplace_back(ValueFactory::GetNullValueByType(plan_->GetRightPlan()->OutputSchema().GetColumn(i).GetType()));
      }
      *tuple=Tuple(output,&GetOutputSchema());
      has_right_tuple_=true;
      return true;
    }

    if(!left_executor_->Next(&left_tuple_,&left_rid_)){
      return false;
      has_done_=true;
    }

    right_executor_->Init();
    has_right_tuple_=false;
  }
}

}  // namespace bustub

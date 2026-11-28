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
    //child也要初始化
    child_executor_->Init();
    
    tableInfo_=exec_ctx_->GetCatalog()->GetTable(plan_->GetTableOid());
    tableHeap_=tableInfo_->table_.get();

    is_called_=false;
}

/*
测试能不能过，还要看这两点
child_executor_ 是否已经创建：
如果 child_executor_ 是在 Init() 里通过 ExecutorFactory::CreateExecutor(...) 构造的，
那 child_executor_->Init() 必须放在创建它之后；如果构造函数里已经 make_unique 好了，
那放 Init() 第一行没问题。

表上有索引时要不要插索引：
插入一条新元组后，如果这张表在 Catalog 里有对应的索引
（exec_ctx_->GetCatalog()->GetTableIndexes(tableInfo_->name_)），
且 InsertTuple 成功拿到了 rid（比如 inserted_rid.has_value()），
通常还需要调 index_info->index_->InsertEntry(...) 把新 (tuple, *inserted_rid) 插进索引，
否则涉及索引扫描或唯一/索引测试可能会漏数据。
*/
auto InsertExecutor::Next(Tuple *tuple, RID *rid) -> bool {
    if(is_called_)  return false;

    //把child里的tuple插入到plan_指向的table里面并计数
    Tuple targetTuple;
    RID targetRid;
    TupleMeta meta{0,false};
    int32_t cnt=0;
    
    while(child_executor_->Next(&targetTuple,&targetRid)){
        std::optional<RID> insertRid=tableHeap_->InsertTuple(meta,targetTuple,exec_ctx_->GetLockManager(),exec_ctx_->GetTransaction(),tableInfo_->oid_);
        if(insertRid.has_value()){
            cnt++;    
        }
    }

    //把得到的结果包装成Tuple传出去
    is_called_=true;
    //Value v(INTEGER,cnt);
    std::vector<Value> vet{Value{TypeId::INTEGER,cnt}};
    *tuple=Tuple{vet,&GetOutputSchema()};
    return true;
}

/*
3. 把整个 Insert Executor 的工作串起来
InsertExecutor
1. 从 Child Executor 获取待插入 Tuple


2. 构造 TupleMeta：temp_ts、未删除


3. 插入 Table Heap，获取 RID


4. 将 RID 加入当前事务 Write Set


5. 等待 Commit 阶段把临时时间戳变成正式提交时间戳


4. 你实际需要重点找哪些东西？
先打开：

src/execution/insert_executor.cpp

重点关注：
接口或成员	作用
child_executor_->Next()	获取待插入数据
TableHeap::InsertTuple()	真正插入 Tuple
TupleMeta	设置时间戳和删除标志
txn->AppendWriteSet()	记录插入的 RID
GetTransactionTempTs()	获取当前事务的临时时间戳
*/

}  // namespace bustub

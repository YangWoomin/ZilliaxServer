
#include    "common/log.h"

#include    "db_test/db_test.h"

#include    <thread>

using TestParamType1 = Params<std::string, int32_t>;
using TestParamType2 = Params<const char*, int32_t>;
using TestParamType3 = Params<int32_t, std::string>;
using TestParamType4 = Params<std::string, float32_t>;
using TestResultType1 = ResultSet<int32_t, std::string, int32_t>;
using TestResultType2 = ResultSet<int32_t, std::string, float32_t>;

/////////////////////////////////////////////////////////////////////////////
// SimpleProcCallOperation
SimpleProcCallOperation::SimpleProcCallOperation(contextID cid)
    : Operation(cid)
{

}

void SimpleProcCallOperation::process()
{
    if (false == Prepare("CALL InsertTestData(?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleProcCallOperation::process, cid : %d", GetContextID());
        return;
    }

    std::string name = "hi7";
    TestParamType1* params = new TestParamType1 {name, int32_t(700)};
    params->SetParamTypes<ParamType::INPUT, ParamType::INPUT>();

    if (false == BindParams(ParamsSPtr(params)))
    {
        ZS_LOG_ERROR(db, "BindParams failed in SimpleProcCallOperation::process, cid : %d", GetContextID());
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleProcCallOperation::process, cid : %d", GetContextID());
        return;
    }

    ZS_LOG_INFO(db, "db test op processed in SimpleProcCallOperation::process, cid : %d", GetContextID());
}

void SimpleProcCallOperation::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleProcCallOperation::process, cid : %d", fr, GetContextID());
}


/////////////////////////////////////////////////////////////////////////////
// SimpleProcCallOperation2
SimpleProcCallOperation2::SimpleProcCallOperation2(contextID cid)
    : Operation(cid)
{

}

void SimpleProcCallOperation2::process()
{
    if (false == Prepare("CALL SelectTestData(?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleProcCallOperation2::process, cid : %d", GetContextID());
        return;
    }

    TestParamType2* params = new TestParamType2 {"hi7", 0};
    params->SetParamTypes<ParamType::INPUT, ParamType::OUTPUT>();

    if (false == BindParams(ParamsSPtr(params)))
    {
        ZS_LOG_ERROR(db, "BindParams failed in SimpleProcCallOperation2::process, cid : %d", GetContextID());
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleProcCallOperation2::process, cid : %d", GetContextID());
        return;
    }

    const TestParamType2::ParamsRaw& pr = params->GetParams();
    int32_t val = std::get<1>(pr);

    ZS_LOG_INFO(db, "db test op processed in SimpleProcCallOperation2::process, value : %d, cid : %d", val, GetContextID());
}

void SimpleProcCallOperation2::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleProcCallOperation2::process, cid : %d", fr, GetContextID());
}


/////////////////////////////////////////////////////////////////////////////
// SimpleProcCallOperation3
SimpleProcCallOperation3::SimpleProcCallOperation3(contextID cid)
    : Operation(cid)
{

}

void SimpleProcCallOperation3::process()
{
    if (false == Prepare("CALL SelectTestData2(?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleProcCallOperation3::process, cid : %d", GetContextID());
        return;
    }

    std::string nameParam;
    nameParam.resize(16, '\0');
    TestParamType3* params = new TestParamType3 { 700, nameParam };
    params->SetParamTypes<ParamType::INPUT, ParamType::OUTPUT>();

    if (false == BindParams(ParamsSPtr(params)))
    {
        ZS_LOG_ERROR(db, "BindParams failed in SimpleProcCallOperation3::process, cid : %d", GetContextID());
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleProcCallOperation3::process, cid : %d", GetContextID());
        return;
    }

    const TestParamType3::ParamsRaw& pr = params->GetParams();
    std::string name = std::get<1>(pr);

    ZS_LOG_INFO(db, "db test op processed in SimpleProcCallOperation3::process, name : %s, cid : %d", name.c_str(), GetContextID());
}

void SimpleProcCallOperation3::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleProcCallOperation3::process, cid : %d", fr, GetContextID());
}


/////////////////////////////////////////////////////////////////////////////
// SimpleSelectCallOperation
SimpleSelectCallOperation::SimpleSelectCallOperation(contextID cid)
    : Operation(cid)
{

}

void SimpleSelectCallOperation::process()
{
    if (false == Prepare("SELECT * FROM test_table"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleSelectCallOperation::process, cid : %d", GetContextID());
        return;
    }

    std::string name (16, '\0');
    TestResultType1* rs = new TestResultType1 { 0, name, 0 };

    if (false == Execute(ResultSetSPtr(rs)))
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleSelectCallOperation::process, cid : %d", GetContextID());
        return;
    }

    const TestResultType1::Result& result = rs->GetResult();
    for (const auto& ele : result)
    {
        ZS_LOG_INFO(db, "result - id : %d, name : %s, value : %d in SimpleSelectCallOperation::process, cid : %d", 
            std::get<0>(ele), std::get<1>(ele).c_str(), std::get<2>(ele), GetContextID());
    }

    ZS_LOG_INFO(db, "db test op processed in SimpleSelectCallOperation::process, cid : %d", GetContextID());
}

void SimpleSelectCallOperation::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleSelectCallOperation::process, cid : %d", fr, GetContextID());
}


/////////////////////////////////////////////////////////////////////////////
// SimpleTransactionalOperation
SimpleTransactionalOperation::SimpleTransactionalOperation(std::string inName, int32_t inVal, std::string upName, int32_t upVal, contextID cid)
    : TransactionalOperation(cid), _inName(inName), _inVal(inVal), _upName(upName), _upVal(upVal)
{

}

void SimpleTransactionalOperation::process()
{
// begin transaction
    if (false == BeginTransaction())
    {
        ZS_LOG_ERROR(db, "BeginTransaction failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        return;
    }

// insert a row
    if (false == Prepare("INSERT INTO test_table (name, value) VALUES (?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare 1 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        return;
    }

    TestParamType1* params1 = new TestParamType1 { _inName, _inVal };
    params1->SetParamTypes<ParamType::INPUT, ParamType::INPUT>();

    if (false == BindParams(ParamsSPtr(params1)))
    {
        ZS_LOG_ERROR(db, "BindParams 1 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute 1 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        return;
    }

// update rows
    if (false == Prepare("UPDATE test_table SET value = ? WHERE name = ?"))
    {
        ZS_LOG_ERROR(db, "Prepare 2 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

    TestParamType3* params2 = new TestParamType3 { _upVal, _upName };
    params2->SetParamTypes<ParamType::INPUT, ParamType::INPUT>();
    if (false == BindParams(ParamsSPtr(params2)))
    {
        ZS_LOG_ERROR(db, "BindParams 2 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute 2 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

// select
    if (false == Prepare("SELECT * FROM test_table"))
    {
        ZS_LOG_ERROR(db, "Prepare 3 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

    _upName.resize(16, '\0');
    TestResultType1* rs = new TestResultType1 { 0, _upName, 0 };

    if (false == Execute(ResultSetSPtr(rs)))
    {
        ZS_LOG_ERROR(db, "Execute 3 failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

// commit transaction
    if (false == CommitTransaction())
    {
        ZS_LOG_ERROR(db, "CommitTransaction failed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
        return;
    }

    const TestResultType1::Result& result = rs->GetResult();
    for (const auto& ele : result)
    {
        ZS_LOG_INFO(db, "result - id : %d, name : %s, value : %d in SimpleTransactionalOperation::process, cid : %d", 
            std::get<0>(ele), std::get<1>(ele).c_str(), std::get<2>(ele), GetContextID());
    }

    ZS_LOG_INFO(db, "db test op processed in SimpleTransactionalOperation::process, cid : %d", GetContextID());
}

void SimpleTransactionalOperation::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleTransactionalOperation::process, cid : %d", fr, GetContextID());
}


/////////////////////////////////////////////////////////////////////////////
// SimpleTransactionalOperation2
SimpleTransactionalOperation2::SimpleTransactionalOperation2(std::string inName, int32_t inVal, std::string upName, float32_t upVal, contextID cid)
    : TransactionalOperation(cid), _inName(inName), _inVal(inVal), _upName(upName), _upVal(upVal)
{

}

void SimpleTransactionalOperation2::process()
{
// begin transaction
    if (false == BeginTransaction())
    {
        ZS_LOG_ERROR(db, "BeginTransaction failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        return;
    }

// insert a row in test_table
    if (false == Prepare("INSERT INTO test_table (name, value) VALUES (?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare 1 failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        return;
    }

    TestParamType1* params1 = new TestParamType1 { _inName, _inVal };
    params1->SetParamTypes<ParamType::INPUT, ParamType::INPUT>();

    if (false == BindParams(ParamsSPtr(params1)))
    {
        ZS_LOG_ERROR(db, "BindParams 1 failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute 1 failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        return;
    }

// insert a row in test_table2
    if (false == Prepare("INSERT INTO test_table2 (name, value) VALUES (?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare 2 failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

    TestParamType4* params2 = new TestParamType4 { _upName, _upVal };
    params2->SetParamTypes<ParamType::INPUT, ParamType::INPUT>();
    if (false == BindParams(ParamsSPtr(params2)))
    {
        ZS_LOG_ERROR(db, "BindParams 2 failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute 2 failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        RollbackTransaction();
        return;
    }

// commit transaction
    if (false == CommitTransaction())
    {
        ZS_LOG_ERROR(db, "CommitTransaction failed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
        return;
    }

    ZS_LOG_INFO(db, "db test op processed in SimpleTransactionalOperation2::process, cid : %d", GetContextID());
}

void SimpleTransactionalOperation2::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleTransactionalOperation2::process, cid : %d", fr, GetContextID());
}

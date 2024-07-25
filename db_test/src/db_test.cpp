
#include    "common/log.h"

#include    "db_test/db_test.h"


/////////////////////////////////////////////////////////////////////////////
// SimpleProcCallOperation
using TestInputType1 = Params<std::string, int32_t>;
using TestInputType2 = Params<const char*, int32_t>;

SimpleProcCallOperation::SimpleProcCallOperation(contextID cid)
    : Operation(cid)
{

}

void SimpleProcCallOperation::process()
{
    if (false == Prepare("CALL InsertTestData(?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleProcCallOperation::process");
        return;
    }

    // std::string name = "hi7";
    // TestInputType1* params = new TestInputType1{name, int32_t(700)};
    TestInputType2* params = new TestInputType2 {"hi7", int32_t(700)};
    params->SetParamTypes<ParamType::INPUT, ParamType::INPUT>();

    if (false == BindParams(ParamsSPtr(params)))
    {
        ZS_LOG_ERROR(db, "BindParams failed in SimpleProcCallOperation::process");
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleProcCallOperation::process");
        return;
    }

    ZS_LOG_INFO(db, "db test op processed in SimpleProcCallOperation::process");
}

void SimpleProcCallOperation::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleProcCallOperation::process", fr);
}


/////////////////////////////////////////////////////////////////////////////
// SimpleProcCallOperation2
using TestInOutType1 = Params<std::string, int32_t>;
using TestInOutType2 = Params<const char*, int32_t>;

SimpleProcCallOperation2::SimpleProcCallOperation2(contextID cid)
    : Operation(cid)
{

}

void SimpleProcCallOperation2::process()
{
    if (false == Prepare("CALL SelectTestData(?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleProcCallOperation2::process");
        return;
    }

    TestInOutType2* params = new TestInOutType2 {"hi7", 0};
    params->SetParamTypes<ParamType::INPUT, ParamType::OUTPUT>();

    if (false == BindParams(ParamsSPtr(params)))
    {
        ZS_LOG_ERROR(db, "BindParams failed in SimpleProcCallOperation2::process");
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleProcCallOperation2::process");
        return;
    }

    const TestInOutType2::ParamsRaw& pr = params->GetParams();
    int32_t val = std::get<1>(pr);

    ZS_LOG_INFO(db, "db test op processed in SimpleProcCallOperation2::process, value : %d", val);
}

void SimpleProcCallOperation2::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleProcCallOperation2::process", fr);
}


/////////////////////////////////////////////////////////////////////////////
// SimpleProcCallOperation3
using TestInOutType3 = Params<int32_t, std::string>;

SimpleProcCallOperation3::SimpleProcCallOperation3(contextID cid)
    : Operation(cid)
{

}

void SimpleProcCallOperation3::process()
{
    if (false == Prepare("CALL SelectTestData2(?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleProcCallOperation3::process");
        return;
    }

    std::string nameParam;
    nameParam.resize(16, NULL);
    //TestInOutType3* params = new TestInOutType3 { 700, std::string{} };
    TestInOutType3* params = new TestInOutType3 { 700, nameParam };
    params->SetParamTypes<ParamType::INPUT, ParamType::OUTPUT>();

    if (false == BindParams(ParamsSPtr(params)))
    {
        ZS_LOG_ERROR(db, "BindParams failed in SimpleProcCallOperation3::process");
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleProcCallOperation3::process");
        return;
    }

    const TestInOutType3::ParamsRaw& pr = params->GetParams();
    std::string name = std::get<1>(pr);

    ZS_LOG_INFO(db, "db test op processed in SimpleProcCallOperation3::process, name : %s", name.c_str());
}

void SimpleProcCallOperation3::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleProcCallOperation3::process", fr);
}


/////////////////////////////////////////////////////////////////////////////
// SimpleSelectCallOperation
using TestResultSetType = ResultSet<int32_t, std::string, int32_t>;

SimpleSelectCallOperation::SimpleSelectCallOperation(contextID cid)
    : Operation(cid)
{

}

void SimpleSelectCallOperation::process()
{
    if (false == Prepare("SELECT * FROM test_table"))
    {
        ZS_LOG_ERROR(db, "Prepare failed in SimpleSelectCallOperation::process");
        return;
    }

    std::string name (16, NULL);
    TestResultSetType* rs = new TestResultSetType { 0, name, 0 };

    if (false == Execute(ResultSetSPtr(rs)))
    {
        ZS_LOG_ERROR(db, "Execute failed in SimpleSelectCallOperation::process");
        return;
    }

    const TestResultSetType::Result& result = rs->GetResult();
    for (const auto& ele : result)
    {
        ZS_LOG_INFO(db, "result - id : %d, name : %s, value : %d in SimpleSelectCallOperation::process", std::get<0>(ele), std::get<1>(ele).c_str(), std::get<2>(ele));
    }

    ZS_LOG_INFO(db, "db test op processed in SimpleSelectCallOperation::process");
}

void SimpleSelectCallOperation::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d in SimpleSelectCallOperation::process", fr);
}

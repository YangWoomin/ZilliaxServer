
#include    "common/log.h"

#include    "db_test/db_test.h"

using TestDataType = Params<std::string, int32_t>;
using TestDataType2 = Params<const char*, int32_t>;

SimpleInsertSelectOperation::SimpleInsertSelectOperation(contextID cid)
    : Operation(cid)
{

}

void SimpleInsertSelectOperation::process()
{
    if (false == Prepare("CALL InsertTestData(?, ?)"))
    {
        ZS_LOG_ERROR(db, "Prepare failed");
        return;
    }

    TestDataType2* params = new TestDataType2{"hi4", int32_t(400)};
    params->SetParamTypes<ParamType::INPUT, ParamType::INPUT>();

    if (false == BindParams(ParamsSPtr(params)))
    {
        ZS_LOG_ERROR(db, "BindParams failed");
        return;
    }

    if (false == Execute())
    {
        ZS_LOG_ERROR(db, "Execute failed");
        return;
    }

    ZS_LOG_INFO(db, "db test op processed");
}

void SimpleInsertSelectOperation::failed(OperationFailureReason fr)
{
    ZS_LOG_ERROR(db, "db test op failed, failure reason : %d", fr);
}


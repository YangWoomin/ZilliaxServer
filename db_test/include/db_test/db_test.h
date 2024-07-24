#pragma

#include    "db/operation.h"
#include    "db/parameter.h"

using namespace zs::common;
using namespace zs::db;

// test 1
class SimpleInsertSelectOperation : public Operation
{
public:
    SimpleInsertSelectOperation(contextID cid);
    virtual ~SimpleInsertSelectOperation() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleInsertSelectOperationSPtr = std::shared_ptr<SimpleInsertSelectOperation>;

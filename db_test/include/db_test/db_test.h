#pragma

#include    "db/operation.h"
#include    "db/parameter.h"
#include    "db/result_set.h"

using namespace zs::common;
using namespace zs::db;

// simple procedure call (only input - std::string/const char*, int32_t)
class SimpleProcCallOperation : public Operation
{
public:
    SimpleProcCallOperation(contextID cid);
    virtual ~SimpleProcCallOperation() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleProcCallOperationSPtr = std::shared_ptr<SimpleProcCallOperation>;

// simple procedure call (input/output - int32_t)
class SimpleProcCallOperation2 : public Operation
{
public:
    SimpleProcCallOperation2(contextID cid);
    virtual ~SimpleProcCallOperation2() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleProcCallOperation2SPtr = std::shared_ptr<SimpleProcCallOperation2>;

// simple procedure call (input/output - std::string/char*)
class SimpleProcCallOperation3 : public Operation
{
public:
    SimpleProcCallOperation3(contextID cid);
    virtual ~SimpleProcCallOperation3() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleProcCallOperation3SPtr = std::shared_ptr<SimpleProcCallOperation3>;

// simple select call (input/output - std::string/char*)
class SimpleSelectCallOperation : public Operation
{
public:
    SimpleSelectCallOperation(contextID cid);
    virtual ~SimpleSelectCallOperation() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleSelectCallOperationSPtr = std::shared_ptr<SimpleSelectCallOperation>;

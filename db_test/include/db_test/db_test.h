#pragma once

#include    "db/operation.h"
#include    "db/parameter.h"
#include    "db/result_set.h"

using namespace zs::common;
using namespace zs::db;

// simple procedure call (only input - std::string, int32_t)
class SimpleProcCallOperation : public Operation
{
public:
    SimpleProcCallOperation(contextID cid);
    virtual ~SimpleProcCallOperation() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleProcCallOperationSPtr = std::shared_ptr<SimpleProcCallOperation>;

// simple procedure call (input/output - const char*, int32_t)
class SimpleProcCallOperation2 : public Operation
{
public:
    SimpleProcCallOperation2(contextID cid);
    virtual ~SimpleProcCallOperation2() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleProcCallOperation2SPtr = std::shared_ptr<SimpleProcCallOperation2>;

// simple procedure call (input/output - int32_t, std::string)
class SimpleProcCallOperation3 : public Operation
{
public:
    SimpleProcCallOperation3(contextID cid);
    virtual ~SimpleProcCallOperation3() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleProcCallOperation3SPtr = std::shared_ptr<SimpleProcCallOperation3>;

// simple select call (result set - int32_t, std::string, int32_t)
class SimpleSelectCallOperation : public Operation
{
public:
    SimpleSelectCallOperation(contextID cid);
    virtual ~SimpleSelectCallOperation() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;
};

using SimpleSelectCallOperationSPtr = std::shared_ptr<SimpleSelectCallOperation>;

// simple transaction test (example of transaction and causing deadlock)
class SimpleTransactionalOperation : public TransactionalOperation
{
public:
    SimpleTransactionalOperation(std::string inName, int32_t inVal, std::string upName, int32_t upVal, contextID cid);
    virtual ~SimpleTransactionalOperation() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;

private:
    std::string _inName;
    int32_t     _inVal;
    std::string _upName;
    int32_t     _upVal;
};

using SimpleTransactionalOperationSPtr = std::shared_ptr<SimpleTransactionalOperation>;

// simple transaction test (example of transaction and no deadlock)
class SimpleTransactionalOperation2 : public TransactionalOperation
{
public:
    SimpleTransactionalOperation2(std::string inName, int32_t inVal, std::string upName, float32_t upVal, contextID cid);
    virtual ~SimpleTransactionalOperation2() = default;

    virtual void process() override;
    virtual void failed(OperationFailureReason fr) override;

private:
    std::string _inName;
    int32_t     _inVal;
    std::string _upName;
    float32_t   _upVal;
};

using SimpleTransactionalOperation2SPtr = std::shared_ptr<SimpleTransactionalOperation2>;

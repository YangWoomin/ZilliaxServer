
#ifndef __ZS_DB_OPERATION_H__
#define __ZS_DB_OPERATION_H__

#if  defined(_MSVC_)
#include    <windows.h>
#endif

#include    <sql.h>
#include    <sqlext.h>
#include	<sqltypes.h>

#include    <memory>
#include    <vector>
#include    <string>

#include    "common/types.h"
#include    "db/parameter.h"
#include    "db/result_set.h"

namespace zs
{
namespace db
{
    using namespace zs::common;

    enum __ZS_DB_API OperationFailureReason : int32_t
    {
        OPERATION_FAILURE_REASON_NONE = 0,
        OPERATION_FAILURE_REASON_DISCONNECTED = 1,

        OPERATION_FAILURE_REASON_MAX
    };

    class __ZS_DB_API Operation
    {
    protected:
        Operation(contextID cid);
        virtual ~Operation() = default;

        contextID GetContextID() const;
        
        /////////////////////////////////////////////////////
        // you can use these functions 
        // to control your database work sequence
        bool Prepare(const char* query);
        bool BindParams(ParamsSPtr params);
        bool Execute(ResultSetSPtr rs = nullptr);
        /////////////////////////////////////////////////////
    
    private:
        void execute(SQLHDBC hDbc, SQLHSTMT hStmt);
        void clearStatement();
        void complete();
        
        /////////////////////////////////////////////////////
        // you should implement these functions 
        // for your database work by using the above functions
        virtual void process() = 0;
        // handling for failure such as dbms disconnection
        virtual void failed(OperationFailureReason fr) = 0;
        /////////////////////////////////////////////////////

    friend class Worker;
    friend class TransactionalOperation;

    private:
        SQLHDBC                     _hDbc;
        SQLHSTMT                    _hStmt;
        std::string                 _query;
        std::vector<ParamsSPtr>     _params;
        ResultSetSPtr               _rs;
        contextID                   _cid;
    };

    class __ZS_DB_API TransactionalOperation : public Operation
    {
    public:
        TransactionalOperation(contextID cid);
        virtual ~TransactionalOperation() = default;

        /////////////////////////////////////////////////////
        // you can use these functions with transaction
        bool BeginTransaction();
        bool RollbackTransaction();
        bool CommitTransaction();
        /////////////////////////////////////////////////////
    };
}
}

#endif // __ZS_DB_OPERATION_H__

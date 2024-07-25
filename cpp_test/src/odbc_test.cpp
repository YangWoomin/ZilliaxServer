
#if defined(_MSVC_)
#include <windows.h>
#endif

#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <iostream>
#include <thread>
#include <string>
#include <mutex>

std::mutex locker;

void show_error(unsigned int handletype, const SQLHANDLE& handle) {
    SQLCHAR SQLState[1024];
    SQLCHAR Message[1024];
    if (SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, SQLState, NULL, Message, 1024, NULL)) {
        std::cout << "SQL error: " << Message << "\nSQL state: " << SQLState << std::endl;
    }
}

void insert_data(int thread_id, const char* value) {
    SQLHANDLE sqlenvhandle;
    SQLHANDLE sqlconnectionhandle;
    SQLHANDLE sqlstatementhandle_x;
    SQLHANDLE sqlstatementhandle_y;
    SQLRETURN retcode;

    locker.lock();

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlenvhandle))
        return;
    if (SQL_SUCCESS != SQLSetEnvAttr(sqlenvhandle, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0))
        return;
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlenvhandle, &sqlconnectionhandle))
        return;

    // 멀티 스레드에 의해 동시접근을 하면 안되나보다 자꾸 크래시난다
    switch (SQLDriverConnect(sqlconnectionhandle, NULL,
        (SQLCHAR*)"DRIVER={MySQL ODBC 9.0 ANSI Driver};SERVER=localhost;DATABASE=testdb;USER=test;PASSWORD=test321;OPTION=3;",
        SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT)) {
    case SQL_SUCCESS_WITH_INFO:
        show_error(SQL_HANDLE_DBC, sqlconnectionhandle);
        break;
    case SQL_INVALID_HANDLE:
    case SQL_ERROR:
        show_error(SQL_HANDLE_DBC, sqlconnectionhandle);
        return;
    default:
        break;
    }

    // 트랜잭션 격리 수준 설정
    if (SQL_SUCCESS != SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_TXN_ISOLATION, (SQLPOINTER)SQL_TXN_SERIALIZABLE, 0)) {
        show_error(SQL_HANDLE_DBC, sqlconnectionhandle);
        return;
    }

    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle_x))
        return;
    if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle_y))
        return;

    locker.unlock();

    const char* insert_query_x = "INSERT INTO table_x (value, thread_id) VALUES (?, ?)";
    const char* insert_query_y = "INSERT INTO table_y (value, thread_id) VALUES (?, ?)";
    
    // 이렇게 격리 수준을 최상으로 높여도 x 테이블의 n번째 행과 y 테이블의 n번째 행은
    // 하나의 트랜잭션으로 insert 되는 것을 보장받지 못한다
    // 어떤 DBA 형님 말씀으로는 아래와 같다
    // 격리라는것은 서로 다른 트랜잭션이 같은 영역을 접근할 때의 이야기이다
    // 이 예제에서는 두 스레드가 같은 영역(행)을 접근(read)하지 않으니 애초에 격리 레벨이 적용되지 않음
    // 격리 레벨 정의를 읽어보면 같은 데이터를 읽는 다른 트랜잭션들간의 처리에 대한 정의를 한다
    // 결국 insert만 하는것은 데이터를 읽어서 공유하지 않으니 격리 레벨이 의미가 없다
    for (int i = 0; i < 100; ++i) {
        // 트랜잭션 시작 (자동 커밋 모드 비활성화)
        if (SQL_SUCCESS != SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0)) {
            show_error(SQL_HANDLE_DBC, sqlconnectionhandle);
            return;
        }

        if (SQL_SUCCESS != SQLPrepare(sqlstatementhandle_x, (SQLCHAR*)insert_query_x, SQL_NTS)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_x);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }

        // 데이터 삽입 (X 테이블)
        if (SQL_SUCCESS != SQLBindParameter(sqlstatementhandle_x, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0, (SQLPOINTER)value, 0, NULL)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_x);
            SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_ROLLBACK);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }
        if (SQL_SUCCESS != SQLBindParameter(sqlstatementhandle_x, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &thread_id, 0, NULL)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_x);
            SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_ROLLBACK);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }

        if (SQL_SUCCESS != SQLExecute(sqlstatementhandle_x)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_x);
            SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_ROLLBACK);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }

        if (SQL_SUCCESS != SQLPrepare(sqlstatementhandle_y, (SQLCHAR*)insert_query_y, SQL_NTS)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_y);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }

        // 데이터 삽입 (Y 테이블)
        if (SQL_SUCCESS != SQLBindParameter(sqlstatementhandle_y, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0, (SQLPOINTER)value, 0, NULL)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_y);
            SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_ROLLBACK);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }
        if (SQL_SUCCESS != SQLBindParameter(sqlstatementhandle_y, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &thread_id, 0, NULL)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_y);
            SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_ROLLBACK);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }

        if (SQL_SUCCESS != SQLExecute(sqlstatementhandle_y)) {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_y);
            SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_ROLLBACK);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }

        // 트랜잭션 커밋
        if (SQL_SUCCESS != SQLEndTran(SQL_HANDLE_DBC, sqlconnectionhandle, SQL_COMMIT)) {
            show_error(SQL_HANDLE_DBC, sqlconnectionhandle);
            SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            return;
        }

        // 트랜잭션 완료 후 자동 커밋 모드 다시 활성화
        if (SQL_SUCCESS != SQLSetConnectAttr(sqlconnectionhandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0)) {
            show_error(SQL_HANDLE_DBC, sqlconnectionhandle);
            return;
        }

        SQLRETURN ret = SQLFreeStmt(sqlstatementhandle_x, SQL_CLOSE);
        if (!SQL_SUCCEEDED(ret))
        {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_x);
            return;
        }

        ret = SQLFreeStmt(sqlstatementhandle_x, SQL_UNBIND);
        if (!SQL_SUCCEEDED(ret))
        {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_x);
            return;
        }

        ret = SQLFreeStmt(sqlstatementhandle_x, SQL_RESET_PARAMS);
        if (!SQL_SUCCEEDED(ret))
        {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_x);
            return;
        }

        ret = SQLFreeStmt(sqlstatementhandle_y, SQL_CLOSE);
        if (!SQL_SUCCEEDED(ret))
        {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_y);
            return;
        }

        ret = SQLFreeStmt(sqlstatementhandle_y, SQL_UNBIND);
        if (!SQL_SUCCEEDED(ret))
        {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_y);
            return;
        }

        ret = SQLFreeStmt(sqlstatementhandle_y, SQL_RESET_PARAMS);
        if (!SQL_SUCCEEDED(ret))
        {
            show_error(SQL_HANDLE_STMT, sqlstatementhandle_y);
            return;
        }
    }

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle_x);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle_y);
    SQLDisconnect(sqlconnectionhandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlconnectionhandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlenvhandle);
}

int odbc_test() {
    // 스레드 생성
    std::thread thread_a(insert_data, 1, "A");
    std::thread thread_b(insert_data, 2, "B");

    // 스레드가 완료될 때까지 기다림
    thread_a.join();
    thread_b.join();

    return 0;
}

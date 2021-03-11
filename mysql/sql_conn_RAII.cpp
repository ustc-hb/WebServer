// RAII方法来获得资源 避免资源泄露
// 以对象管理资源
#include <mysql/mysql.h>
#include "sql_conn_RAII.h"

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}
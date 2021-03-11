// 以对象管理资源
#ifndef SQL_CONN_RAII_H
#define SQL_CONN_RAII_H

#include "sql_connection_pool.h"

class connectionRAII{
public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};


#endif
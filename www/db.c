/*
 * db.c
 *
 * Copyright (C) 2011-2012	OpenTech Labs
 *				Andrew Clayton <andrew@opentechlabs.co.uk>
 * Released under the GNU Affero General Public License version 3.
 * See COPYING
 */

#include <stdlib.h>
#include <stdarg.h>
#include <netdb.h>

#include "common.h"
#include "receiptomatic_config.h"
#include "url_helpers.h"
#include "utils.h"
#include "db.h"

char *db_host = "localhost";
char *db_socket_name = NULL;
unsigned int db_port_num = 3306;
unsigned int db_flags = 0;

/*
 * Opens up a MySQL connection and returns the connection handle.
 */
MYSQL *db_conn(void)
{
	MYSQL *conn;
	MYSQL *ret;

	if (MULTI_TENANT) {
		char tenant[NI_MAXHOST];
		char db[NI_MAXHOST + 3] = "rm_";

		get_tenant(env_vars.host, tenant);
		strncat(db, tenant, NI_MAXHOST);
		free(db_name);
		db_name = strdup(db);
	}
	conn = mysql_init(NULL);
	ret = mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME,
			DB_PORT_NUM, DB_SOCKET_NAME, DB_FLAGS);

	if (!ret) {
		d_fprintf(error_log, "Failed to connect to database. Error: "
				"%s\n", mysql_error(conn));
		switch (mysql_errno(conn)) {
		case ER_BAD_DB_ERROR:	/* unknown database */
			send_template("templates/invalid.tmpl", NULL, NULL);
			break;
		}
		conn = NULL;
	}

	return conn;
}

/*
 * Make sure we have a good db connection. The most likely case where this
 * will fail is in multi-tenancy mode when someones uses an invalid hostname
 * and there won't be a database to match.
 */
bool check_db_conn(void)
{
	MYSQL *conn;
	bool ret = false;

	conn = db_conn();
	if (conn)
		ret = true;

	mysql_close(conn);
	return ret;
}

/*
 * This takes a mysql connection and sql query and returns the result set.
 * It also takes __func__ to get the name of the calling function. It also
 * logs the query into the sql log.
 *
 * This function should not be called directly and should instead be used via
 * the sql_query() macro.
 *
 * This function will either return a result set or NULL. Note that some
 * queries don't return result sets by design.
 */
MYSQL_RES *__sql_query(const char *func, MYSQL *conn, char *fmt, ...)
{
	va_list args;
	char sql[SQL_MAX];
	char tenant[NI_MAXHOST];
	struct timespec tp;
	int len;

	va_start(args, fmt);
	len = vsnprintf(sql, sizeof(sql), fmt, args);
	va_end(args);

	get_tenant(env_vars.host, tenant);
	clock_gettime(CLOCK_REALTIME, &tp);
	fprintf(sql_log, "%ld.%06ld %d %s %s: %s\n", tp.tv_sec,
			tp.tv_nsec / NS_USEC, getpid(), tenant, func, sql);
	fflush(sql_log);

	mysql_real_query(conn, sql, len);
	return mysql_store_result(conn);
}

/*
 * audit.c - Auditing subsystem
 *
 * Copyright (C) 2011-2013	OpenTech Labs
 *				Andrew Clayton <andrew@digital-domain.net>
 *
 * 		 2016		Andrew Clayton <andrew@digital-domain.net>
 *
 * Released under the GNU Affero General Public License version 3.
 * See COPYING
 */

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <glib.h>

#include "common.h"
#include "utils.h"

/*
 * Add a login entry to the utmp table.
 *
 * We log the time (seconds.microseconds), uid, username, ip address,
 * hostname and the session id that was assigned to this session.
 */
unsigned long long log_login(void)
{
	char *username;
	char *hostname;
	char *ip_addr;
	char host[NI_MAXHOST] = "\0";
	struct addrinfo hints;
	struct addrinfo *ares;
	struct timespec login_at;
	unsigned long long sid;
	unsigned int uid;
	MYSQL_RES *res;
	MYSQL_ROW row;

	clock_gettime(CLOCK_REALTIME, &login_at);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	getaddrinfo(env_vars.remote_addr, NULL, &hints, &ares);
	getnameinfo(ares->ai_addr, ares->ai_addrlen, host, NI_MAXHOST, NULL, 0,
			0);

	username = make_mysql_safe_string(get_var(qvars, "username"));
	hostname = make_mysql_safe_string(host);
	ip_addr = make_mysql_safe_string(env_vars.remote_addr);
	res = sql_query("SELECT uid FROM passwd WHERE username = '%s'",
			username);
	row = mysql_fetch_row(res);
	uid = atoi(row[0]);
	mysql_free_result(res);

	/* We need to be sure a new sid isn't inserted here */
	sql_query("LOCK TABLES utmp WRITE");
	res = sql_query("SELECT IFNULL(MAX(sid), 0) FROM utmp");
	row = mysql_fetch_row(res);

	sid = strtoull(row[0], NULL, 10) + 1;

	/* Divide tv_nsec by 1000 to get a rough microseconds value */
	sql_query("INSERT INTO utmp VALUES (%ld.%06ld, %u, '%s', '%s', %d, "
			"'%s', %llu)",
			login_at.tv_sec, login_at.tv_nsec / NS_USEC,
			uid, username, ip_addr, env_vars.remote_port,
			hostname, sid);
	sql_query("UNLOCK TABLES");

	mysql_free_result(res);
	free(username);
	free(hostname);
	free(ip_addr);
	freeaddrinfo(ares);

	return sid;
}

/*
 * Retrieves the last login time and the host the login came from
 * for a given user.
 *
 * If the user has never logged in before, 0 is returned.
 */
time_t get_last_login(char *from_host)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	time_t login;

	/*
	 * We need to; ORDER BY login_at DESC LIMIT 1, 1
	 * due to the login being logged before we get the last login.
	 * This ensures we actually get the last login and not the
	 * current login.
	 *
	 * If the user has never logged in before, we will get an empty row.
	 */
	res = sql_query("SELECT login_at, hostname FROM utmp WHERE uid = %u "
			"ORDER BY login_at DESC LIMIT 1, 1", user_session.uid);
	if (mysql_num_rows(res) > 0) {
		row = mysql_fetch_row(res);
		login = atol(row[0]);
		snprintf(from_host, NI_MAXHOST, "%s", row[1]);
	} else {
		login = 0;
	}
	mysql_free_result(res);

	return login;
}

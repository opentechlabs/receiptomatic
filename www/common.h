/*
 * common.h
 *
 * Copyright (C) 2011-2012	OpenTech Labs
 *				Andrew Clayton <andrew@opentechlabs.co.uk>
 * Released under the GNU Affero General Public License version 3.
 * See COPYING
 */

#ifndef _COMMON_H_
#define _COMMON_H_

/*
 * Lets not pretend we care about running this on anything
 * other than Linux.
 *
 * On Linux, MySQL defines _GNU_SOURCE 1 in my_global.h, but
 * lets do it explicitly here anyway.
 */
#define _GNU_SOURCE 1

#ifdef _RECEIPTOMATIC_WWW_
#include <fcgi_stdio.h>
#endif

#include <sys/time.h>

#include <glib.h>

/* HTML template library */
#include <ctemplate.h>

#include "receiptomatic_config.h"
#include "db.h"

#define GRID_SIZE	9
#define ROW_SIZE	3
#define COL_SIZE	3

#define APPROVER_ROWS	3	/* No. of rows / page on /approve_receipts/ */

#define APPROVER		(1 << 0)	/*   1 */
#define APPROVER_SELF		(1 << 1)	/*   2 */
#define APPROVER_CASH		(1 << 2)	/*   4 */
#define APPROVER_CARD		(1 << 3)	/*   8 */
#define APPROVER_CHEQUE		(1 << 4)	/*  16 */
#define ADMIN			(1 << 7)	/* 128 */

#define BUF_SIZE	4096
#define SQL_MAX		8192

#define REJECTED	0
#define PENDING		1
#define APPROVED	2

#define SHA256		 5
#define SHA512		10

/* Macro to check if the user is an approver */
#define IS_APPROVER()	(((user_session.capabilities) & (APPROVER)) ? 1 : 0)
/* Macro to check if the user is an admin */
#define IS_ADMIN()	(((user_session.capabilities) & (ADMIN)) ? 1 : 0)

/*
 * Wrapper around fprintf(). It will prepend the text passed it with
 * seconds.microseconds pid function:
 *
 * e.g if you call it like: d_fprintf(debug, "This is a test\n");
 * You will get:
 *
 * 1304600723.663486 1843 main: This is a test
 */
#define d_fprintf(stream, fmt, ...) \
	do { \
		struct timeval tv; \
		gettimeofday(&tv, NULL); \
		fprintf(stream, "%ld.%ld %d %s: " fmt, tv.tv_sec, tv.tv_usec, \
					getpid(), __func__, ##__VA_ARGS__); \
		fflush(stream); \
	} while (0)

/*
 * Structure that defines a users session. The session is stored
 * in a tokyocabinet database table inbetween requests.
 */
struct session {
	unsigned long long sid;
	unsigned int uid;
	unsigned char capabilities;
	char *username;
	char *name;
	time_t login_at;
	time_t last_seen;
	char *origin_ip;
	char *client_id;
	char *session_id;
	char *csrf_token;
	unsigned int restrict_ip;
	char *user_hdr;
} session;
extern struct session user_session;

struct field_names {
	char *receipt_date;
	char *department;
	char *employee_number;
	char *reason;
	char *po_num;
	char *cost_codes;
	char *account_codes;
	char *supplier_name;
	char *supplier_town;
	char *vat_number;
	char *gross_amount;
	char *net_amount;
	char *vat_amount;
	char *vat_rate;
	char *currency;
	char *payment_method;
} field_names;

/*
 * This structure maps to the environment variable list sent
 * by the application. We don't store every item.
 */
struct env_vars {
	char *request_uri;
	char *request_method;
	char *content_type;
	char *http_cookie;
	char *http_user_agent;
	char *http_x_forwarded_for;
	char *query_string;
} env_vars;
extern struct env_vars env_vars;

/* Default Field Names */
#define DFN_RECEIPT_DATE	"Receipt Date"
#define DFN_DEPARTMENT		"Department"
#define DFN_EMPLOYEE_NUMBER	"Employee Number"
#define DFN_REASON		"Reason"
#define DFN_PO_NUM		"PO Num"
#define DFN_COST_CODES		"Cost Code(s)"
#define DFN_ACCOUNT_CODES	"Account Code(s)"
#define DFN_SUPPLIER_NAME	"Supplier Name"
#define DFN_SUPPLIER_TOWN	"Supplier Town"
#define DFN_VAT_NUMBER		"VAT Number"
#define DFN_GROSS_AMOUNT	"Gross Amount"
#define DFN_NET_AMOUNT		"Net Amount"
#define DFN_VAT_AMOUNT		"VAT Amount"
#define DFN_VAT_RATE		"VAT Rate"
#define DFN_CURRENCY		"Currency"
#define DFN_PAYMENT_METHOD	"Payment Method"

extern FILE *access_log;
extern FILE *sql_log;
extern FILE *error_log;
extern FILE *debug_log;

extern struct file_info file_info;
extern GList *u_files;
extern GList *avars;
extern GHashTable *qvars;

#endif /* _COMMON_H_ */

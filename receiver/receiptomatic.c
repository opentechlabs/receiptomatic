/*
 * receiptomatic.c
 *
 * Copyright (C) 2011-2012	OpenTech Labs
 * 				Andrew Clayton <andrew@digital-domain.net>
 *
 * Released under the GNU Affero General Public License version 3.
 * See COPYING
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <netdb.h>

#include <mhash.h>

#include <magick/api.h>

#include <gmime/gmime.h>

#include <glib.h>

#include <my_global.h>
#include <mysql.h>

#include "../www/receiptomatic_config.h"
#include "../www/get_config.h"


#define BUF_SIZE	4096
#define SQL_MAX		8192

#define IMG_MEDIUM	1
#define IMG_SMALL	0

/* dummy declarations for extern declarations in receiptomatic_config.h */
char *log_dir;
char *sql_log;
int debug_level;

char *db_host = "localhost";
char *db_socket_name = NULL;
unsigned int db_port_num = 3306;
unsigned int db_flags = 0;

struct email_headers {
	char *from;
	char *to;
} email_headers;

/*
 * Opens up a MySQL connection and returns the connection handle.
 */
static MYSQL *db_conn(void)
{
	MYSQL *conn;
	MYSQL *ret;

	conn = mysql_init(NULL);
	ret = mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME,
			DB_PORT_NUM, DB_SOCKET_NAME, DB_FLAGS);

	return conn;
}

static int do_config(void)
{
	int ret;

	ret = access("/usr/local/etc/receiptomatic.cfg", R_OK);
	if (ret == 0) {
		get_config("/usr/local/etc/receiptomatic.cfg");
		goto out;
	}

	ret = access("/etc/receiptomatic.cfg", R_OK);
	if (ret == 0) {
		get_config("/etc/receiptomatic.cfg");
		goto out;
	}

	ret = access("./receiptomatic.cfg", R_OK);
	if (ret == 0) {
		get_config("./receiptomatic.cfg");
		goto out;
	}

out:
	return ret;
}

static void send_error_email(const char *email_addr)
{
	static bool sent_email;

	/* We only want to send one error email per received email */
	if (sent_email)
		return;

	FILE *fp = popen(MAIL_CMD, "w");

	fprintf(fp, "Reply-To: %s\r\n", MAIL_REPLY_TO);
	fprintf(fp, "From: %s\r\n", MAIL_FROM);
	fprintf(fp, "Subject: Receiptomatic email error\r\n");
	fprintf(fp, "To: %s\r\n", email_addr);
	fputs("Content-Type: text/plain; charset=us-ascii\r\n", fp);
	fputs("Content-Transfer-Encoding: 7bit\r\n", fp);
	fputs("\r\n", fp);

	fputs("The email address that you sent your image(s) from is not the "
			"one you\r\nuse to log into the system.\r\n", fp);
	fputs("\r\n", fp);
	fputs("Your image(s) have _not_ been stored.\r\n", fp);

	pclose(fp);
	sent_email = true;
}

/*
 * Given an email address in any of the following forms:
 *
 * 	John Doe <j.doe@example.com>
 * 	<j.doe@example.com>
 * 	j.doe@example.com
 *
 * return
 *
 * 	j.doe@example.com
 */
static char *get_email_addr(const char *addr)
{
	char *email;

	if (!strstr(addr, " ")) {
		if (strstr(addr, "<") && strstr(addr, ">")) {
			/* Handle: <j.doe@example.com> */
			email = malloc(strlen(addr));
			strncpy(email, addr + 1, strlen(addr) - 2);
		} else {
			/* Handle: j.doe@example.com */
			email = malloc(strlen(addr) + 1);
			strcpy(email, addr);
		}
	} else {
		/* Handle: John Doe <j.doe@example.com> */
		char *string;
		char *token;

		string = strdupa(addr);

		token = strtok(string, "<");
		token = NULL;
		token = strtok(token, "<");

		token[strlen(token) - 1] = '\0';

		email = malloc(strlen(addr));
		strcpy(email, token);
	}

	return email;
}

/*
 * Given an email address in the form; receipts@<tenant>.domain
 *
 * It will return the tenant part.
 */
static void  get_tenant(const char *email_addr, char *tenant)
{
	char *email;
	char *token;
	char *string;

	email = get_email_addr(email_addr);
	string = strdupa(email);
	free(email);

	token = strtok(string, "@");
	token = NULL;
	token = strtok(token, ".");

	snprintf(tenant, sizeof(tenant), "%s", token);
}

/*
 * Generate a SHA-256 of a given image
 */
static char *create_image_id(char *path, char *filename)
{
	int fd;
	int dirfd;
	ssize_t bytes_read;
	int i;
	int hbs;
	char buf[BUF_SIZE];
	unsigned char *hash;
	char ht[3];
	char shash[65];
	MHASH td;

	td = mhash_init(MHASH_SHA256);
	dirfd = open(path, O_RDONLY);
	fd = openat(dirfd, filename, O_RDONLY);
	do {
		bytes_read = read(fd, &buf, BUF_SIZE);
		mhash(td, &buf, bytes_read);
	} while (bytes_read > 0);
	close(fd);
	close(dirfd);
	hash = mhash_end(td);

	printf("Hash: ");
	memset(shash, 0, sizeof(shash));
	hbs = mhash_get_block_size(MHASH_SHA256);
	for (i = 0; i < hbs; i++) {
		sprintf(ht, "%.2x", hash[i]);
		strncat(shash, ht, 2);
	}
	printf("%s\n", shash);

	free(hash);

	return strdup(shash);
}

static void resize_image(const char *path, const char *filename, int size)
{
	ExceptionInfo exception;
	Image *image;
	Image *new_image;
	ImageInfo *image_info;
	char output_file[PATH_MAX];
	int x;
	int y;
	int err;

	err = chdir(path);
	if (err == -1)
		exit(EXIT_FAILURE);

	if (size == IMG_SMALL) {
		mkdir("small", 0777);
		snprintf(output_file, PATH_MAX, "small/%s", filename);
		x = 180;
		y = 180;
	} else if (size == IMG_MEDIUM) {
		mkdir("medium", 0777);
		snprintf(output_file, PATH_MAX, "medium/%s", filename);
		x = 300;
		y = 300;
	} else {
		goto out;
	}
	printf("Creating image: %s\n", output_file);

	InitializeMagick(NULL);
	GetExceptionInfo(&exception);
	image_info = CloneImageInfo((ImageInfo *)NULL);
	strcpy(image_info->filename, filename);
	image = ReadImage(image_info, &exception);
	new_image = ResizeImage(image, x, y, LanczosFilter, 1.0, &exception);
	DestroyImage(image);
	strcpy(new_image->filename, output_file);
	WriteImage(image_info, new_image);

	DestroyImageInfo(image_info);
	DestroyMagick();

out:
	err = chdir("/tmp");
	if (err == -1)
		exit(EXIT_FAILURE);
}

/*
 * Save the attached image to the filesystem
 */
static void save_image(GMimeObject *part, const char *path,
							const char *filename)
{
	int fd;
	int dirfd;
	GMimeStream *stream;
	GMimeDataWrapper *content;

	dirfd = open(path, O_RDONLY);
	fd = openat(dirfd, filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	stream = g_mime_stream_fs_new(fd);
	content = g_mime_part_get_content_object((GMimePart *)part);
	g_mime_data_wrapper_write_to_stream(content, stream);
	g_mime_stream_flush(stream);

	g_object_unref(content);
        g_object_unref(stream);

	close(dirfd);
}

/*
 * Process a MIME part of the mail message
 */
static void process_part(GMimeObject *part, struct email_headers *eh)
{
	const GMimeContentType *content_type;
	char ymd[11];	/* YYYY/MM/DD */
	char filename[NAME_MAX + 1];
	char ext[5];
	char path[PATH_MAX];
	char sql[SQL_MAX];
	char *user;
	char *from;
	char *image_id;
	int bytes;
	unsigned int uid;
	time_t t;
	DIR *dir;
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;

	printf("Part: %s\n", (char *)g_mime_part_get_filename(
							(GMimePart *)part));
	content_type = g_mime_object_get_content_type(part);
	printf("Content-Type: %s\n",
				g_mime_content_type_to_string(content_type));
	if (strcmp(g_mime_content_type_to_string(content_type),
							"image/jpeg") == 0)
		strcpy(ext, ".jpg");
	else if (strcmp(g_mime_content_type_to_string(content_type),
							"image/png") == 0)
		strcpy(ext, ".png");
	else
		return;

	conn = db_conn();
	/*
	 * Determine the path where to store the image.
	 *
	 *	[tenant/]UID/YYYY/MM/DD
	 */
	from = get_email_addr(eh->from);
	user = alloca(strlen(eh->from)*2 + 1);
	mysql_real_escape_string(conn, user, from, strlen(from));
	free(from);
	if (MULTI_TENANT) {
		char tenant[NI_MAXHOST];
		char db[NI_MAXHOST + 3] = "rm_";

		get_tenant(eh->to, tenant);
		strncat(db, tenant, NI_MAXHOST);
		free(db_name);
		db_name = strdup(db);
		fprintf(stderr, "Set db name to %s\n", db_name);
	}
	mysql_close(conn);

	/* conn should now point to either receiptomatic or 'tenant' */
	conn = db_conn();
	snprintf(sql, sizeof(sql), "SELECT uid FROM passwd WHERE username = "
								"'%s'", user);
	printf("SQL: %s\n", sql);
	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	if (mysql_num_rows(res) == 0) {
		send_error_email(eh->from);
		goto out;
	}
	row = mysql_fetch_row(res);
	uid = atoi(row[0]);

	t = time(NULL);
	strftime(ymd, sizeof(ymd), "%Y/%m/%d", localtime(&t));
	bytes = snprintf(path, PATH_MAX, "%s/%s%s%u/%s", IMAGE_PATH,
			(MULTI_TENANT) ? db_name + 3 : "",
			(MULTI_TENANT) ? "/" : "", uid, ymd);
	if (bytes >= PATH_MAX)
		goto out;
	printf("Path: %s\n", path);

	dir = opendir(path);
	if (!dir) {
		g_mkdir_with_parents(path, 0777);
		strcpy(filename, "000-receipt_image");
		strcat(filename, ext);
	} else {
		struct dirent64 *entry;
		int ne = 0;

		while ((entry = readdir64(dir)) != NULL) {
			/* skip . .. medium & small entries */
			if (entry->d_name[0] == '.' ||
						entry->d_name[0] == 'm' ||
						entry->d_name[0] == 's')
				continue;
			ne++;
		}
		closedir(dir);

		sprintf(filename, "%03d-receipt_image%s", ne, ext);
	}

	printf("Creating file: %s/%s\n", path, filename);
	save_image(part, path, filename);
	resize_image(path, filename, IMG_SMALL);
	resize_image(path, filename, IMG_MEDIUM);

	image_id = create_image_id(path, filename);

	/* In the database we only store the path relative from IMAGE_PATH */
	sprintf(path, "%s", path + strlen(IMAGE_PATH)+1);
	snprintf(sql, SQL_MAX,
		"INSERT INTO images VALUES ('%s', %u, '%s', %ld, '%s', '%s', "
						"0, 1)",
						image_id, uid, user, t, path,
						filename);
	printf("SQL: %s\n", sql);
	mysql_query(conn, sql);

	free(image_id);

out:
	mysql_free_result(res);
	mysql_close(conn);
}

/*
 * Process a mail message, passed on by sendmail
 */
static void process_message(int dirfd, const char *filename)
{
	GMimeMessage *message;
	GMimeStream *stream;
	GMimeParser *parser;
	const InternetAddressList *recips;
	InternetAddress *addr;
	int fd;
	struct email_headers *eh;

	g_mime_init(0);

	fd = openat(dirfd, filename, O_RDONLY);
	stream = g_mime_stream_fs_new(fd);
	parser = g_mime_parser_new_with_stream(stream);
	message = g_mime_parser_construct_message(parser);
	recips = g_mime_message_get_recipients(message,
						GMIME_RECIPIENT_TYPE_TO);
	addr = internet_address_list_get_address(recips);
	printf("From: %s\n", (char *)g_mime_message_get_sender(message));
	printf("To: %s\n", internet_address_to_string(addr, FALSE));
	printf("Subject: %s\n", (char *)g_mime_message_get_subject(message));

	eh = malloc(sizeof(struct email_headers));
	eh->from = (char *)g_mime_message_get_sender(message);
	eh->to = internet_address_to_string(addr, FALSE);
	g_mime_message_foreach_part(message, (GMimePartFunc)process_part,
				(struct email_headers *)eh);
	free(eh);

	g_object_unref(stream);
	g_object_unref(parser);
	g_object_unref(message);

	g_mime_shutdown();
}

int main(int argc, char **argv)
{
	int fd;
	int dirfd;
	ssize_t bytes_read;
	ssize_t bytes_wrote;
	char buf[BUF_SIZE];
	char temp_name[] = "receiptomatic-XXXXXX";
	int err;

	if (argc == 2) {
		get_config(argv[1]);
	} else {
		err = do_config();
		if (err == -1) {
			fprintf(stderr, "Could not open config file.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* Be super restrictive for the tempfile creation */
	umask(0077);
	err = chdir("/tmp");
	if (err == -1)
		exit(EXIT_FAILURE);

	/*
	 * This program gets the mail message pipe'd to it from sendmail.
	 *
	 * This means we need to copy the mail message to a temporary file
	 * or the gmime stuff will fail due to operating on a pipe
	 * (illegal seek).
	 */
	fd = mkstemp(temp_name);
	if (fd == -1)
		exit(EXIT_FAILURE);
	do {
		bytes_read = read(STDIN_FILENO, &buf, BUF_SIZE);
		bytes_wrote = write(fd, buf, bytes_read);
		if (bytes_wrote != bytes_read)
			exit(EXIT_FAILURE);
	} while (bytes_read > 0);
	close(fd);

	umask(0007);
	dirfd = open("/tmp", O_RDONLY);
	process_message(dirfd, temp_name);

	unlinkat(dirfd, temp_name, 0);
	close(dirfd);

	exit(EXIT_SUCCESS);
}

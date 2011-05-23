/*
 * data_extraction.h
 *
 * Copyright (C) 2011		OpenTech Labs
 * 				Andrew Clayton <andrew@opentechlabs.co.uk>
 * Released under the GNU General Public License (GPL) version 3.
 * See COPYING
 */

#ifndef _DATA_EXTRACTION_H_
#define _DATA_EXTRACTION_H_

extern void send_receipt_data(int fd);
extern void extract_data_now(struct session *current_session, int fd);

#endif /* _DATA_EXTRACTION_H_ */
/**********************************************************************
* Author:	jaron.ho
* Date:		2017-12-25
* Brief:	logfile
**********************************************************************/
#ifndef _LOGFILE_H_
#define _LOGFILE_H_

#include <stdio.h>

#ifdef LOGFILE_THREAD_SAFETY
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct logfile_st {
    FILE* fileptr;
    char* filename;
    size_t maxsize;
    unsigned int enable;
#ifdef LOGFILE_THREAD_SAFETY
    pthread_mutex_t mutex;
#endif
} logfile_st;

/*
 * Brief:	open a logfile
 * Param:	filename - log file name
 *			maxSize - file max size
 * Return:	logfile_st*
 */
extern logfile_st* logfile_open(const char* filename, size_t maxSize);

/*
 * Brief:	close a logfile
 * Param:	lf - a log file
 * Return:	void
 */
extern void logfile_close(logfile_st* lf);

/*
 * Brief:	clear logfile
 * Param:	lf - a log file
 * Return:	0.ok
 *          1.clear lf fail, can not open file
 */
extern unsigned int logfile_clear(logfile_st* lf);

/*
 * Brief:	get a logfile name
 * Param:	lf - a log file
 * Return:	const char*
 */
extern const char* logfile_name(logfile_st* lf);

/*
 * Brief:	get logfile enable status
 * Param:	lf - a log file
 * Return:	0.disable
 *          1.enable
 */
extern unsigned int logfile_isenable(logfile_st* lf);

/*
 * Brief:	set logfile enable status
 * Param:	lf - a log file
 *          enable - 0.false, 1.true
 * Return:	void
 */
extern void logfile_enable(logfile_st* lf, unsigned int enable);

/*
 * Brief:	record to log file
 * Param:	lf - a log file
 *          content - record content
 *          newline - whether create a new line, 0.false, 1.true
 * Return:	0.ok
 *          1.lf is disable
 *          2.content size large max
 *          3.file size reach max
 */
extern unsigned int logfile_record(logfile_st* lf, const char* content, unsigned int newline);

/*
 * Brief:	record to log file with time
 * Param:	lf - a log file
 *          content - record content
 * Return:	0.ok
 *          1.lf is disable
 *          2.content size large max
 *          3.file size reach max
 */
extern unsigned int logfile_record_with_time(logfile_st* lf, const char* content);

/*
 * Brief:	record to log file with time and tag
 * Param:	lf - a log file
 *          tag - record tag
 *          withtime - with time, 0.false, 1.true
 *          content - record content
 * Return:	0.ok
 *          1.lf is disable
 *          2.content size large max
 *          3.file size reach max
 */
extern unsigned int logfile_record_with_tag(logfile_st* lf, const char* tag, unsigned int withtime, const char* content);

#ifdef __cplusplus
}
#endif

#endif	// _LOGFILE_H_

#ifndef UTILS_H_
#define UTILS_H_

#ifdef _WIN32_

#include <winsock.h>
#include <time.h>

#else

#include <sys/time.h>
#include <unistd.h>

#endif

#include <sys/timeb.h>

#include <cstdlib>

#ifdef _TSCI2_
extern "C" {
#include "../Include/tsci2.h"
}
#endif

void utils_init();

#ifdef _WIN32_

void utils_close();

LARGE_INTEGER getFILETIMEoffset();

/*
 * Sleep com a precisão de microsegundos.
 */
void time_usleep(struct timeval *t2sleep);

/*
 * Sleep com a precisão de microsegundos.
 * ex:
 *		2.5 -> 2 segundos e 500.000 microsegundos
 */
void time_usleepd(double db);

#else

void time_usleep(struct timeval *t2sleep);

#endif

void time_getus(struct timeval *tm);

long time_getnow_sec();

long time_getnow_usec();

double utils_rand();

void time_add(struct timeval *t1, struct timeval *t2, struct timeval *tr);

void time_subtract(struct timeval *t1, struct timeval *t2, struct timeval *tr);

void time_add_ms(struct timeval *t1, long ms, struct timeval *tr);

/*
 * Multiplica um timeval por uma constante.
 */
void time_mul (struct timeval *t, double c, struct timeval *tr);

/*
 * Divide um timeval por uma constante.
 */
void time_div (struct timeval *t, double c, struct timeval *tr);

/*
 * Retorna:
 *	   -1	se t1 <  t2
 *		0	se t1 == t2
 *		1	se t1 >  t2
 */
int time_compare(struct timeval *t1, struct timeval *t2);

/*
 * Retorna:
 *	   -1	se t <  us
 *		0	se t == us
 *		1	se t >  us
 */
int time_compare_us(struct timeval *t, long us);

/*
 * Retorna:
 *	   -1	se t <  s
 *		0	se t == s
 *		1	se t >  s
 */
int time_compare_s(struct timeval *t, long s);

/*
 * Converte timeval para double.
 * ex: se timeval->tv_sec == 2, timeval->tv_usec == 500000
 *	   retorna 2.5
 */
double time_timeval2double(struct timeval *t);

/*
 * Converte timeval para double em microsegundos.
 */
double time_timeval2double_us(struct timeval *t);

/*
 * Converte de double para timeval.
 * Parametro tem de estar em microsegundos.
 * ex: db == 2.5,
 *	   retorna timeval->tv_sec = 2, timeval->tv_usec = 500000
 */
struct timeval time_double2timeval(double db);

#endif /*UTILS_H_*/


#include "Utils.h"

#ifdef _WIN32_

SOCKET Sock_uSleep;
fd_set Sock_uSleep_Set;

#else

struct timeval timestart;

#endif

#ifdef _WIN32_

void utils_init()
{
	//Socket utilizado na função select() nas funções de sleep
	Sock_uSleep = socket(AF_INET, SOCK_DGRAM, 0);
	struct timeval time;
    FD_ZERO(&Sock_uSleep_Set);
    FD_SET(Sock_uSleep,&Sock_uSleep_Set);
}

void utils_close()
{
	closesocket(Sock_uSleep);
}

#else

void utils_init()
{
#ifdef _TSCI2_
	int t_method, t_methods;
	t_methods = TSCI2_DAEMON | TSCI2_CLIENT | TSCI2_FALLBACK;
	t_method = tsci2_init(t_methods);
	tsci2_gettimeofday(&timestart, NULL);
#else
	gettimeofday(&timestart, NULL);
#endif
}

#endif

#ifdef _WIN32_

void time_getus(struct timeval *tm)
{
	LARGE_INTEGER			t;
	FILETIME               f;
	double                 microseconds;
	static LARGE_INTEGER   offset;
	static double          frequencyToMicroseconds;
	static int             initialized = 0;
	static BOOL            usePerformanceCounter = 0;

	if (!initialized) 
	{
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
		if (usePerformanceCounter) {
			QueryPerformanceCounter(&offset);
			frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
		} else {
			offset = getFILETIMEoffset();
			frequencyToMicroseconds = 10.;
		}
	}
	if (usePerformanceCounter) QueryPerformanceCounter(&t);
	else {
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = microseconds;

	tm->tv_sec = t.QuadPart / 1000000;
	tm->tv_usec = t.QuadPart % 1000000;

}

LARGE_INTEGER getFILETIMEoffset()
{              
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;
	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return (t);
}

#else

void time_getus(struct timeval *tm)
{
#ifdef _TSCI2_
	tsci2_gettimeofday(tm, NULL);
#else
	gettimeofday(tm, NULL);
#endif
	time_subtract(tm, &timestart, tm);

}

#endif

long time_getnow_sec()
{
	struct timeval t;
	time_getus(&t);
	return t.tv_sec;
}

long time_getnow_usec()
{
	struct timeval t;
	time_getus(&t);
	return t.tv_usec;
}

double utils_rand()
{
	struct timeb t;
	ftime(&t);
	struct timeval t2;
	time_getus(&t2);
	int x = (int)(t2.tv_usec/1000.0);
	double x1 = (t2.tv_usec/1000.0 - x)*1000;
	return (((double)t.millitm + x1)/(1998.0))*RAND_MAX;
}

void time_add(struct timeval *t1, struct timeval *t2, struct timeval *tr)
{
	tr->tv_sec = t1->tv_sec + t2->tv_sec;
	tr->tv_usec = t1->tv_usec + t2->tv_usec;
	while (tr->tv_usec >= 1000000)
	{
		tr->tv_usec -= 1000000;
		tr->tv_sec += 1;
	}
}

void time_add_ms(struct timeval *t1, long ms, struct timeval *tr)
{
	tr->tv_sec = t1->tv_sec;
	tr->tv_usec = t1->tv_usec + ms*1000;
	while (tr->tv_usec >= 1000000)
	{
		tr->tv_usec -= 1000000;
		tr->tv_sec += 1;
	}
}

void time_subtract(struct timeval *t1, struct timeval *t2, struct timeval *tr)
{
	tr->tv_sec = t1->tv_sec - t2->tv_sec;
	tr->tv_usec = t1->tv_usec - t2->tv_usec;
	while ((tr->tv_usec < 0) && (tr->tv_sec > 0)) 
	{
			tr->tv_sec -= 1;
			tr->tv_usec += 1000000;
	}
}

void time_mul(struct timeval *t, double c, struct timeval *tr)
{
	double dbt;
	dbt = time_timeval2double(t);
	dbt *= c;
	struct timeval taux;
	taux = time_double2timeval(dbt);		
	tr->tv_sec = taux.tv_sec;
	tr->tv_usec = taux.tv_usec;
}

void time_div(struct timeval *t, double c, struct timeval *tr)
{
	double dbt;
	dbt = time_timeval2double(t);
	dbt = dbt/c;
	struct timeval taux;
	taux = time_double2timeval(dbt);
	tr->tv_sec = taux.tv_sec;
	tr->tv_usec = taux.tv_usec;
}

int time_compare(struct timeval *t1, struct timeval *t2)
{
	if (t1->tv_sec < t2->tv_sec) return -1;
	if (t1->tv_sec > t2->tv_sec) return 1;
	if (t1->tv_sec == t2->tv_sec) 
	{
		if (t1->tv_usec < t2->tv_usec) return -1;
		if (t1->tv_usec > t2->tv_usec) return 1;
		if (t1->tv_usec == t2->tv_usec) return 0;
	}
	return 0; // apenas para não gerar warning
}

int time_compare_us(struct timeval *t, long us)
{
	if (us < 0) return 1;
	long seg = 0;
	long useg = us;
	while (useg >= 1000000) {
		seg += 1;
		useg -= 1000000;
	} 
	if (t->tv_sec > seg) return 1;
	if (t->tv_sec < seg) return -1;
	if (t->tv_sec == seg) 
	{
		if (t->tv_usec > useg) return 1;
		if (t->tv_usec < useg) return -1;
		if (t->tv_usec == useg) return 0;
	}
	return 0; // apenas para não gerar warning
}

int time_compare_s(struct timeval *t, long s)
{
	if (t->tv_sec > s) return 1;
	if (t->tv_sec < s) return -1;
	if (t->tv_sec == s) return 0;
	return 0; // apenas para não gerar warning
}

double time_timeval2double_us(struct timeval *t)
{
	return (t->tv_sec*1000000 + t->tv_usec);
}

double time_timeval2double(struct timeval *t)
{
	return (t->tv_sec + t->tv_usec*0.000001);
}

struct timeval time_double2timeval(double db)
{
	struct timeval te;
	te.tv_sec = 0;
	te.tv_usec = 0;		
	while (db >= 1) 
	{
		te.tv_sec += 1;
		db -= 1;
	}
	te.tv_usec = (long)(db*1000000);
	return te;
}

#ifdef _WIN32_

void time_usleepd(double db)
{
	struct timeval t;
	t = time_double2timeval(db);
	time_usleep(&t);
}

void time_usleep(struct timeval *t2sleep)
{	
	FD_SET(Sock_uSleep,&Sock_uSleep_Set);
    int r = select(0,NULL,NULL,&Sock_uSleep_Set,t2sleep);
}

#else

void time_usleep(struct timeval *t2sleep)
{	
	double db = 0.0;
	if (t2sleep->tv_sec > 0) {
		sleep(t2sleep->tv_sec);
		if (t2sleep->tv_usec > 0) {
			db = time_timeval2double_us(t2sleep);
			while (db >= 1000000)
				   db -= 1000000;
			usleep((long)db);
		}
	} else {
		db = time_timeval2double_us(t2sleep);
		usleep((long)db);
	}
		
}

#endif

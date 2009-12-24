/* dostime.h - function prototypes */

#ifndef dostime_H
#define dostime_H 1

#ifdef __cplusplus
extern "C" {
#endif

extern U_EXPORT time_t        dos2unixtime(unsigned long dostime);
extern U_EXPORT unsigned long unix2dostime(time_t*);

#ifdef __cplusplus
}
#endif

#endif

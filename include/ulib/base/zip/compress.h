/* compress.h - header for compression */

#ifndef compress_H
#define compress_H 1

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes the compression data structure(s) */
extern U_EXPORT int init_compression(void);

/* Compresses the file specified by in_fd and appends it to out_fd */
extern U_EXPORT int compress_file(int in_fd, int out_fd, struct zipentry* ze);

/* Frees memory used by compression function */
extern U_EXPORT int end_compression(void);

extern U_EXPORT int init_inflation(void);
extern U_EXPORT int inflate_file(pb_file*, int out_fd, struct zipentry* ze);
extern U_EXPORT int inflate_buffer(pb_file*, unsigned* inlen, char** out_buff, unsigned* outlen, struct zipentry* ze);

#ifdef __cplusplus
}
#endif

#endif

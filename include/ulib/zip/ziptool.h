/* ziptool.h */

#ifdef __cplusplus
extern "C" {
#endif

extern U_EXPORT int zip_archive(const char* zipfile, const char* files[]); /* create new archive */
extern U_EXPORT int   zip_match(const char* zipfile, const char* files[]); /* match archive list with named files */

/* extract named (or all) files from archive */
extern U_EXPORT unsigned zip_extract(const char* zipfile, const char** files, char*** filenames, unsigned** filenames_len);

/* get content of all files from archive data */
extern U_EXPORT unsigned zip_get_content(const char* zipdata, unsigned datalen, char*** filenames,    unsigned** filenames_len,
                                                                                char*** filecontents, unsigned** filecontents_len);

#ifdef __cplusplus
}
#endif

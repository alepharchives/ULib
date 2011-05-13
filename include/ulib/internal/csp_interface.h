/* csp_interface.h */

extern char*        get_reply(void);
extern unsigned int get_reply_capacity(void);

extern unsigned int u_url_decode(            char* s, unsigned int n, unsigned char* result, int no_line_break);
extern unsigned int u_xml_decode(            char* s, unsigned int n, unsigned char* result);
extern unsigned int u_base64_decode(         char* s, unsigned int n, unsigned char* result);
extern unsigned int u_url_encode(   unsigned char* s, unsigned int n, unsigned char* result, char* extra_enc_chars);
extern unsigned int u_xml_encode(   unsigned char* s, unsigned int n, unsigned char* result);
extern unsigned int u_base64_encode(unsigned char* s, unsigned int n, unsigned char* result);

extern unsigned int u_snprintf(char* buffer, unsigned int  buffer_size, char* format, ...);

/* TODO: please add (it's boring...)

0000000000044370 T u_atexit
000000000003faf0 T u_basename
0000000000046ac0 T u_calcRate
00000000000497d0 T u_canonicalize_pathname
00000000001dbc30 T u_debug_at_exit
0000000000048820 T u_delimit_token
00000000000541d0 T u_des3_decode
0000000000054010 T u_des3_encode
0000000000053cd0 T u_des3_init
0000000000053fb0 T u_des3_key
0000000000053f70 T u_des3_reset
0000000000054180 T u_des_decode
0000000000053fc0 T u_des_encode
0000000000053be0 T u_des_init
0000000000053f80 T u_des_key
0000000000053ed0 T u_des_reset
0000000000054690 T u_dgst_algoritm
0000000000054a70 T u_dgst_finish
0000000000054560 T u_dgst_get_algoritm
0000000000054380 T u_dgst_hexdump
0000000000054800 T u_dgst_init
00000000000548c0 T u_dgst_reset
0000000000054b80 T u_dgst_sign_finish
0000000000054ae0 T u_dgst_sign_init
0000000000054cd0 T u_dgst_verify_finish
0000000000054b30 T u_dgst_verify_init
0000000000044b30 T u_dosmatch
0000000000048c10 T u_dosmatch_with_OR
0000000000047f60 T u_endsWith
000000000004f6e0 T u_escape_decode
000000000004f100 T u_escape_decode_ptr
000000000004ed80 T u_escape_encode
0000000000044920 T u_execOnExit
000000000003fab0 T u_exit
0000000000047230 T u_find
0000000000046610 T u_findEndHeader
00000000000482a0 T u_find_char
00000000000543d0 t u_finish
000000000004a760 T u_fnmatch
0000000000049db0 T u_ftw
000000000004a440 t u_ftw_call
0000000000049da0 T u_ftw_ino_cmp
0000000000044840 T u_getExitStatus
0000000000046340 T u_getPathRelativ
0000000000046ab0 T u_getScreenWidth
0000000000044670 T u_getSysError
0000000000044770 T u_getSysSignal
000000000004c040 T u_get_mimetype
000000000004a600 T u_get_num_random
0000000000043d60 T u_getcwd
0000000000044210 T u_getsuffix
00000000000534f0 T u_gz_deflate
00000000000537d0 T u_gz_inflate
0000000000044a90 T u_hash
0000000000046300 T u_inet_nltoa
0000000000046310 T u_inet_nstoa
0000000000043f60 T u_init
000000000004b130 T u_isBase64
000000000004b730 T u_isBinary
0000000000049250 T u_isMacAddr
0000000000047f90 T u_isNumber
000000000004b3c0 T u_isText
0000000000049080 T u_isURL
000000000004b650 T u_isUTF16
000000000004b480 T u_isUTF8
000000000004b300 T u_isWhiteSpace
0000000000045b60 T u_memcpy
000000000004a720 T u_passwd_cb
0000000000049b20 T u_pathfind
0000000000044600 T u_printError
0000000000046e50 T u_printSize
000000000003fba0 T u_printf
000000000004df10 T u_quoted_printable_decode
000000000004dcc0 T u_quoted_printable_encode
00000000000449e0 T u_random
0000000000044af0 T u_random64
0000000000046ef0 T u_rmatch
00000000000467d0 T u_runAsUser
000000000003fb20 T u_setPid
00000000000485a0 T u_skip
000000000004b790 T u_split
000000000004bbd0 T u_splitCommand
000000000004ec10 T u_sprintc
0000000000047f30 T u_startsWith
0000000000045790 T u_strcpy
000000000003ffc0 T u_strftime
0000000000045690 T u_strlen
0000000000049490 T u_strnatcmp
0000000000045f30 T u_strncpy
00000000000475b0 T u_strpbrk
00000000000478e0 T u_strpend
00000000000444b0 T u_unatexit
0000000000049320 T u_validate_email_address
0000000000041a30 T u_vsnprintf
*/

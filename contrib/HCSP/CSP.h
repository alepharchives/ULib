/* CSP.h */

typedef unsigned int WORD;
typedef unsigned long DWORD;
typedef DWORD *PDWORD,*LPDWORD;
typedef unsigned char BYTE;
typedef BYTE *PBYTE,*LPBYTE;
typedef unsigned int UINT,*PUINT,*LPUINT;
typedef int WINBOOL,*PWINBOOL,*LPWINBOOL;
typedef WINBOOL BOOL;
typedef char CHAR;
typedef CHAR *PCHAR,*LPCH,*PCH,*NPSTR,*LPSTR,*LPCSTR,*PSTR;
typedef void *HANDLE;
typedef UINT ALG_ID;
typedef unsigned long ULONG;
typedef ULONG HCRYPTPROV;
typedef ULONG HCRYPTKEY;
typedef ULONG HCRYPTHASH;
typedef void* HCERTSTORE;
typedef struct _CERT_CONTEXT {
    DWORD                   dwCertEncodingType;
    BYTE                    *pbCertEncoded;
    DWORD                   cbCertEncoded;
 /* PCERT_INFO              pCertInfo; */
    HCERTSTORE              hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;
typedef struct _PUBLICKEYSTRUC {
        BYTE    bType;
        BYTE    bVersion;
        WORD    reserved;
        ALG_ID  aiKeyAlg;
} BLOBHEADER, PUBLICKEYSTRUC;
typedef struct _RSAPUBKEY {
        DWORD   magic;                  // Has to be RSA1
        DWORD   bitlen;                 // # of bits in modulus
        DWORD   pubexp;                 // public exponent
                                        // Modulus data follows
} RSAPUBKEY;

/*
typedef wchar_t WCHAR;
typedef WCHAR * PWCHAR, * LPWCH, * PWCH, * NWPSTR, * LPWSTR, * PWSTR;
*/

#define IN
#define OUT
#define OPTIONAL
#define WINAPI
#define FALSE 0
#define TRUE 1
#define CP_ACP 0
#define MS_DEF_PROV "Microsoft Base Cryptographic Provider v1.0"
#define PROV_RSA_FULL 1
#define ALG_CLASS_HASH  32768
#define ALG_TYPE_ANY 0
#define ALG_SID_MD5 3
#define ALG_SID_SHA 4
#define CALG_MD5 (ALG_CLASS_HASH|ALG_TYPE_ANY|ALG_SID_MD5)
#define CALG_SHA (ALG_CLASS_HASH|ALG_TYPE_ANY|ALG_SID_SHA)
#define AT_KEYEXCHANGE  1
#define AT_SIGNATURE    2
#define HP_HASHVAL 0x0002  /* Hash value */
#define X509_ASN_ENCODING   0x00000001
#define PKCS_7_ASN_ENCODING 0x00010000
#define CERT_FIND_SUBJECT_STR_A 458759
#define CERT_FIND_SUBJECT_STR CERT_FIND_SUBJECT_STR_A
#define PUBLICKEYBLOB 6
#define PP_KEYEXCHANGE_PIN 32
#define PP_SIGNATURE_PIN   33
#define CRYPT_MACHINE_DEFAULT   0x00000001
#define CRYPT_USER_DEFAULT      0x00000002
#define CERT_STORE_PROV_SYSTEM ((LPCSTR) 10)
#define CERT_SYSTEM_STORE_USERS_ID              6
#define CERT_SYSTEM_STORE_CURRENT_USER_ID       1
#define CERT_SYSTEM_STORE_LOCATION_SHIFT        16
#define CERT_SYSTEM_STORE_CURRENT_USER          \
    (CERT_SYSTEM_STORE_CURRENT_USER_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_STORE_READONLY_FLAG                        0x00008000
#define CERT_STORE_OPEN_EXISTING_FLAG                   0x00004000
#define NTE_BAD_UID (0x80090001L)
#define NTE_BAD_HASH (0x80090002L)
#define NTE_BAD_KEY (0x80090003L)
#define NTE_BAD_LEN (0x80090004L)
#define NTE_BAD_DATA (0x80090005L)
#define NTE_BAD_SIGNATURE (0x80090006L)
#define NTE_BAD_VER (0x80090007L)
#define NTE_BAD_ALGID (0x80090008L)
#define NTE_BAD_FLAGS (0x80090009L)
#define NTE_BAD_TYPE (0x8009000AL)
#define NTE_BAD_KEY_STATE (0x8009000BL)
#define NTE_BAD_HASH_STATE (0x8009000CL)
#define NTE_NO_KEY (0x8009000DL)
#define NTE_NO_MEMORY (0x8009000EL)
#define NTE_EXISTS (0x8009000FL)
#define NTE_PERM (0x80090010L)
#define NTE_NOT_FOUND (0x80090011L)
#define NTE_DOUBLE_ENCRYPT (0x80090012L)
#define NTE_BAD_PROVIDER (0x80090013L)
#define NTE_BAD_PROV_TYPE (0x80090014L)
#define NTE_BAD_PUBLIC_KEY (0x80090015L)
#define NTE_BAD_KEYSET (0x80090016L)
#define NTE_PROV_TYPE_NOT_DEF (0x80090017L)
#define NTE_PROV_TYPE_ENTRY_BAD (0x80090018L)
#define NTE_KEYSET_NOT_DEF (0x80090019L)
#define NTE_KEYSET_ENTRY_BAD (0x8009001AL)
#define NTE_PROV_TYPE_NO_MATCH (0x8009001BL)
#define NTE_SIGNATURE_FILE_BAD (0x8009001CL)
#define NTE_PROVIDER_DLL_FAIL (0x8009001DL)
#define NTE_PROV_DLL_NOT_FOUND (0x8009001EL)
#define NTE_BAD_KEYSET_PARAM (0x8009001FL)
#define NTE_FAIL (0x80090020L)
#define NTE_SYS_ERR (0x80090021L)

#define ERROR_NOT_ENOUGH_MEMORY 8L
#define ERROR_INVALID_PARAMETER 87L

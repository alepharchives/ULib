// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    timestamp.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_TIMESTAMP_H
#define U_TIMESTAMP_H 1

#include <ulib/ssl/pkcs7.h>

#include <openssl/ts.h>

/* A time-stamp token is obtained by sending the digest value of the given data to the Time-Stamp Authority (TSA).
 * The returned time-stamp token is a signed data that contains the digest value, the identity of the TSA, and
 * the time of stamping. This proves that the given data existed before the time of stamping.
 *
 * NOTE: the term time-stamp token used does NOT refer to the TSA's response to a requesting client, but the
 * token generated by the TSA, which is present within this response. In the case of RFC 3161 [10] protocol,
 * the time-stamp token term is referring to the timeStampToken field within the TimeStampResp element
 * (the TSA's response returned to the requesting client).
 */

class Url;

class U_EXPORT UTimeStamp : public UPKCS7 {
public:

   /**
   * Constructs this object from the response of TSA service encoded string.
   *
   * @param string of bytes
   */

   static TS_RESP* readTimeStampResponse(const UString& x);

   UTimeStamp(const UString& x) : UPKCS7(0,0)
      {
      U_TRACE_REGISTER_OBJECT(0, UTimeStamp, "%.*S", U_STRING_TO_TRACE(x))

      response = readTimeStampResponse(x);

      if (response) pkcs7 = (PKCS7*) U_SYSCALL(TS_RESP_get_token, "%p", response);
      }

   UTimeStamp(UString& request, Url& TSA);

   /**
   * Deletes this object.
   */

   ~UTimeStamp()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTimeStamp)

      if (response)
         {
         U_SYSCALL_VOID(TS_RESP_free, "%p", response);

         pkcs7 = 0;
         }
      }

   // VARIE

   bool isValid() const
      {
      U_TRACE(0, "UTimeStamp::isValid()")

      U_RETURN(response != 0);
      }

   static bool isTimeStampToken(PKCS7* p7);
   static bool isTimeStampResponse(const UString& content);

   static UString getTimeStampToken(int alg, const UString& data, const UString& url);
   static UString createQuery(int alg, const UString& data, const char* policy, bool bnonce, bool bcert);

   // DEBUG

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   TS_RESP* response;

private:
   UTimeStamp(const UTimeStamp&) : UPKCS7(0,0) {}
   UTimeStamp& operator=(const UTimeStamp&)    { return *this; }
};

#endif

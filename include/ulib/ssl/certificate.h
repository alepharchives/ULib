// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    certificate.h - interface to a X509 certificate
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_CERTIFICATE_H
#define ULIB_CERTIFICATE_H 1

#include <ulib/date.h>
#include <ulib/string.h>

#include <openssl/x509.h>

template <class T> class UVector;
template <class T> class UHashMap;

/**
  * This class provides all the services the openssl X509 certificate supports.
  * This class contains a openssl X509 structure and basically acts as a wrapper to functions that act on that structure
  */

class U_EXPORT UCertificate {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   /**
   * Constructs this object takes <i>X509</i> as type
   */ 

   UCertificate(X509* px509 = 0) : x509(px509)
      {
      U_TRACE_REGISTER_OBJECT(0, UCertificate, "%p", px509)
      }

   /**
   * Constructs this object from the a encoded string.
   * The <i>type</i> specifies the type of encoding the string is in, e.g. DER or PEM
   *
   * @param encoding a string of bytes
   * @param type the Certificate's encoding type
   */

   static X509* readX509(const UString& x, const char* format = 0);

   UCertificate(const UString& x, const char* format = 0)
      {
      U_TRACE_REGISTER_OBJECT(0, UCertificate, "%.*S,%S", U_STRING_TO_TRACE(x), format)

      x509 = readX509(x, format);
      }

   /**
   * Deletes this object
   */

   void clear()
      {
      U_TRACE(0, "UCertificate::clear()")

      U_INTERNAL_ASSERT_POINTER(x509)

      U_SYSCALL_VOID(X509_free, "%p", x509);

      x509 = 0;
      }

   ~UCertificate()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UCertificate)

      if (x509) clear();
      }

   // VARIE

   bool isValid() const
      {
      U_TRACE(0, "UCertificate::isValid()")

      U_RETURN(x509 != 0);
      }

   bool isSelfSigned() const { return isIssued(*this); }

   bool isSameIssuerAndSubject() const;

   void set(X509* px509)
      {
      U_TRACE(0, "UCertificate::set(%p)", px509)

      if (isValid()) clear();

      if (x509) x509 = X509_dup(px509);
      }

   /**
   * Duplicate this object
   */

   void duplicate()
      {
      U_TRACE(0, "UCertificate::duplicate()")

      x509 = X509_dup(x509);
      }

   X509* getX509() const { return x509; }

   /**
   * Returns the file name from CApath
   */

   static UString getFileName(long hash, bool crl = false, bool* exist = 0);

   static UString getFileName(X509* x509)
      {
      U_TRACE(0, "UCertificate::getFileName(%p)", x509)

      U_INTERNAL_ASSERT_POINTER(x509)

      long hash = U_SYSCALL(X509_subject_name_hash, "%p", x509);

      return getFileName(hash);
      }

   UString getFileName() const { return getFileName(x509); }

   /**
   * Indicate if the certificate has been issued by a CA
   * @param  ca the CA we want to know if it has issued the certificate
   * @return true if ca has issued the certificate, else false
   */

   bool isIssued(const UCertificate& ca) const;

   /**
   * Returns <i>CA</i> that signed this certificate
   */

   static UString getName(X509_NAME* n, bool ldap = false);

   static UString getIssuer(X509* x509, bool ldap = false)
      {
      U_TRACE(1, "UCertificate::getIssuer(%p,%b)", x509, ldap)

      U_INTERNAL_ASSERT_POINTER(x509)

      X509_NAME* n = (X509_NAME*) U_SYSCALL(X509_get_issuer_name, "%p", x509);

      return getName(n, ldap);
      }

   UString getIssuer(bool ldap = false) const { return getIssuer(x509, ldap); }

   UString getIssuerForLDAP() const { return getIssuer(true); }

   /**
   * Returns <i>subject</i> of this certificate
   */

   static UString getSubject(X509* x509, bool ldap = false)
      {
      U_TRACE(1, "UCertificate::getSubject(%p,%b)", x509, ldap)

      U_INTERNAL_ASSERT_POINTER(x509)

      X509_NAME* n = (X509_NAME*) U_SYSCALL(X509_get_subject_name, "%p", x509);

      return getName(n, ldap);
      }

   UString getSubject(bool ldap = false) const { return getSubject(x509, ldap); }

   UString getSubjectForLDAP() const { return getSubject(true); }

   /**
   * Returns the <i>versionNumber</i> of this certificate
   */

   static long getVersionNumber(X509* a)
      {
      U_TRACE(1, "UCertificate::getVersionNumber(%p)", a)

      U_INTERNAL_ASSERT_POINTER(a)

      long version = U_SYSCALL(X509_get_version, "%p", a);

      U_RETURN(version);
      }

   long getVersionNumber() const { return getVersionNumber(x509); }

   /**
   * Returns the <i>serialNumber</i> of this certificate, which together with
   * the issuing CA's name, uniquely identifies this certificate
   */

   static long getSerialNumber(X509* a)
      {
      U_TRACE(0, "UCertificate::getSerialNumber(%p)", a)

      U_INTERNAL_ASSERT_POINTER(a)

      long serial = ASN1_INTEGER_get( X509_get_serialNumber(a) );

      U_RETURN(serial);
      }

   long getSerialNumber() const { return getSerialNumber(x509); }

   static UString checkForSerialNumber(long number);
   static UString checkForSerialNumber(const char* str)
      {
      U_TRACE(0, "UCertificate::checkForSerialNumber(%S)", str)

      U_INTERNAL_ASSERT_POINTER(str)

      return checkForSerialNumber(strtol(str, 0, 0));
      }

   static long hashCode(X509* a)
      {
      U_TRACE(1, "UCertificate::hashCode(%p)", a)

      U_INTERNAL_ASSERT_POINTER(a)

      long hash = U_SYSCALL(X509_subject_name_hash, "%p", a);

      U_RETURN(hash);
      }

   long hashCode() { return hashCode(x509); }

   /**
   * Returns the <i>signature</i> of this certificate
   */

   static UString getSignature(X509* a)
      {
      U_TRACE(0, "UCertificate::getSignature(%a)")

      U_INTERNAL_ASSERT_POINTER(a)

      UString signature( (const char*) a->signature->data,
                                       a->signature->length );

      U_RETURN_STRING(signature);
      }

   UString getSignature() const { return getSignature(x509); }

   /**
   * Returns <i>signatureAlgorithm</i> of this certificate
   */ 

   static UString getSignatureAlgorithm(X509* a)
      {
      U_TRACE(0, "UCertificate::getSignatureAlgorithm(%p)")

      U_INTERNAL_ASSERT_POINTER(a)

      UString signature_algorithm( OBJ_nid2sn( OBJ_obj2nid(a->sig_alg->algorithm) ) );

      U_RETURN_STRING(signature_algorithm);
      }

   UString getSignatureAlgorithm() const { return getSignatureAlgorithm(x509); }

   /**
   * Returns the part of this certificate that is signed
   */ 

   static UString getSignable(X509* x509);

   UString getSignable() const { return getSignable(x509); }

   /**
   * Returns <i>publicKey</i> of this certificate
   */

   static EVP_PKEY* getSubjectPublicKey(X509* a)
      {
      U_TRACE(1, "UCertificate::getSubjectPublicKey(%p)", a)

      U_INTERNAL_ASSERT_POINTER(a)

      EVP_PKEY* evp = (EVP_PKEY*) U_SYSCALL(X509_get_pubkey, "%p", a);

      U_RETURN_POINTER(evp, EVP_PKEY);
      }

   EVP_PKEY* getSubjectPublicKey() const { return getSubjectPublicKey(x509); }

   /**
   * Returns if a private key matches the public key of this certificate
   */

#ifdef HAVE_OPENSSL_98
   static bool matchPrivateKey(X509* a, EVP_PKEY* privateKey)
      {
      U_TRACE(1, "UCertificate::matchPrivateKey(%p,%p)", a, privateKey)

      U_INTERNAL_ASSERT_POINTER(a)

      EVP_PKEY* publicKey = (EVP_PKEY*) U_SYSCALL(X509_get_pubkey, "%p", a);

      bool result = (U_SYSCALL(EVP_PKEY_cmp, "%p,%p", publicKey, privateKey) == 1);

      U_RETURN(result);
      }

   bool matchPrivateKey(EVP_PKEY* privateKey) const { return matchPrivateKey(x509, privateKey); }
#endif

   /**
   * Returns the earliest time that the certificate is valid
   */

   const char* getNotBefore() const
      {
      U_TRACE(0, "UCertificate::getNotBefore()")

      U_INTERNAL_ASSERT_POINTER(x509)

      ASN1_UTCTIME* utctime = X509_get_notBefore(x509);

      U_RETURN((const char*)utctime->data);
      }

   /**
   * Returns the latest time that the certificate is valid
   */

   const char* getNotAfter() const
      {
      U_TRACE(0, "UCertificate::getNotAfter()")

      U_INTERNAL_ASSERT_POINTER(x509)

      ASN1_UTCTIME* utctime = X509_get_notAfter(x509);

      U_RETURN((const char*)utctime->data);
      }

   /**
   * Checks validity of this certificate comparing to current time
   */

   bool checkValidity() const
      {
      U_TRACE(1, "UCertificate::checkValidity()")

      (void) U_SYSCALL(gettimeofday, "%p,%p", &u_now, 0);

      bool result = checkValidity(u_now.tv_sec);

      U_RETURN(result);
      }

   /**
   * Checks validity of this certificate comparing to given time
   */

   bool checkValidity(time_t time) const
      {
      U_TRACE(0, "UCertificate::checkValidity(%ld)", time)

      /*
      Not Before: Jan 25 11:54:00 2005 GMT                                                                                         
      Not After : Jan 25 11:54:00 2006 GMT                                                                                         
      */

      bool result = (time >= UDate::getSecondFromTime(getNotBefore(), true, "%s %2d %2d:%2d:%2d %4d GMT") &&
                     time <= UDate::getSecondFromTime(getNotAfter(),  true, "%s %2d %2d:%2d:%2d %4d GMT"));

      U_RETURN(result);
      }

   /**
   * Returns bool value to indicate the correctness of this certificate
   */

   bool verify(EVP_PKEY* publicKey) const
      {
      U_TRACE(0, "UCertificate::verify(%p)", publicKey)

      U_INTERNAL_ASSERT_POINTER(x509)

      if (U_SYSCALL(X509_verify, "%p,%p", x509, publicKey) <= 0) U_RETURN(false);

      U_RETURN(true);
      }

   static X509_STORE_CTX* csc;

   bool verify(STACK_OF(X509)* chain = 0, time_t certsVerificationTime = 0) const;

   /**
    * Retrieves the signer's certificates from this certificate,
    * it does check their validity and whether any signatures are valid.
    */

   static bool verify_result;

   unsigned getSignerCertificates(UVector<UCertificate*>& vec, STACK_OF(X509)* chain, time_t certsVerificationTime) const;

   /**
   * Returns either the DER or PEM or BASE64 encoding of the certificate depending on the value of format
   */

   UString getEncoded(const char* format = "PEM", int max_columns = 0) const;

   /**
   * Returns the Modulus base64 of the certificate public key
   */

   UString getModulus(int max_columns = 0) const;

   /**
   * Returns the Exponent base64 of the certificate public key
   */

   UString getExponent(int max_columns = 0) const;

   /**
   * Gets X509v3 extensions as array of X509Ext objects
   */

   int getExtensions(UHashMap<UString>& table) const;

   /**
   * Returns <i>RevocationURL</i> for the CA that issued this certificate
   */

   UString getRevocationURL(const char* ext_id = LN_crl_distribution_points) const; // "X509v3 CRL Distribution Points"

   /**
   * Indicate the data certificate ("issuer","serial") for logging...
   */

   static void setForLog(X509* a, UString& buffer, const char* fmt = " (\"%.*s\",\"%ld\")")
      {
      U_TRACE(0, "UCertificate::setForLog(%p,%p,%S)", a, &buffer, fmt)

      UString issuer = getIssuer(a, true);

      buffer.snprintf_add(fmt, U_STRING_TO_TRACE(issuer), getSerialNumber(a));
      }

   void setForLog(UString& buffer, const char* fmt = " (\"%.*s\",\"%ld\")") const { setForLog(x509, buffer, fmt); }

   static STACK_OF(X509)* loadCerts(const UString& content);

   static bool isEqual(X509* a, X509* b)
      {
      U_TRACE(0, "UCertificate::isEqual(%p,%p)", a, b)

      int rc = U_SYSCALL(X509_cmp, "%p,%p", a, b);

      U_RETURN(rc == 0);
      }

   bool operator==(const UCertificate& c) const { return  isEqual(x509, c.x509); }
   bool operator!=(const UCertificate& c) const { return !isEqual(x509, c.x509); }

   // STREAM

   UString print(unsigned long nmflag = 0, unsigned long cflag = 0) const;

   friend U_EXPORT ostream& operator<<(ostream& os, const UCertificate& c);

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   X509* x509;

   static UVector<UCertificate*>* vcert;

   static int verifyCallback(int ok, X509_STORE_CTX* ctx);

private:
   UCertificate(const UCertificate&)            {}
   UCertificate& operator=(const UCertificate&) { return *this; }
};

#endif

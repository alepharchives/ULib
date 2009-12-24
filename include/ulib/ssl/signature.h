// ============================================================================
//
// = LIBRARY
//    ulib - c++ library
//
// = FILENAME
//    signature.h - digital signature
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_SIGNATURE_H
#define ULIB_SIGNATURE_H 1

#include <ulib/string.h>

#include <openssl/evp.h>

  /**
  * This class implements the digital signature holding a ENV_MD_CTX object
  * and wrapping the appropriate openssl functions.
  *
  * A <i>USignature</i> object is provides applications, albeit indirectly,
  * with the functionality of a digital signature algorithm such as RSA-MD5
  * or DSA-SHA1. Digital signatures are used for authentication and integrity
  * assurance of digital data.
  *
  * A <i>USignature</i> object is initialized by calling either the <i>
  * intSign</i> if we are signing or the <i>initVerify</i> method if we are
  * verifying. The data is processed through it using the <i>update</i> methods.
  * Once all the data has been updated, one of the <i>sign</i> methods will be
  * should be called to compute the signature, or the </i>verify</i> method
  * should be called to verify a signature.
  *
  * <PRE>
  * <b>Usage:</b>
  *    USignature sig = USignature("RSA-MD5");
  *
  *    sig->initSign(privateKey);
  *
  *    sig->update("Date");
  *    sig->update("to");
  *    sig->update("be signed");
  *
  *    UString signature = sig->sign();
  * </PRE>
  *
  */

class U_EXPORT USignature {
public:

   // Check for memory error
   U_MEMORY_TEST

   // = Allocator e Deallocator.
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   /**
   * Returns <i>true</i> if the signature algorithm specified
   * by <i>alg</i> is supported.
   *
   * @param alg string name of the algorithm
   */

   static bool isSignatureAlgorithmSupported(const char* alg)
      {
      U_TRACE(0, "USignature::isSignatureAlgorithmSupported(%S)", alg)

      bool result = (OBJ_txt2nid((char*)alg) != NID_undef); // No Such Algorithm

      U_RETURN(result);
      }

   static const EVP_MD* getEVP_MD(const char* alg)
      {
      U_TRACE(0, "USignature::getEVP_MD(%S)", alg)

      U_ASSERT(isSignatureAlgorithmSupported(alg))

      int nid           = OBJ_txt2nid((char*)alg);
      const EVP_MD* evp = EVP_get_digestbynid(nid);

      U_RETURN_POINTER(evp, const EVP_MD);
      }

   /**
   * Constructs this object.
   */

   USignature(const char* alg = "RSA-SHA1") : algorithm(alg)
      {
      U_TRACE_REGISTER_OBJECT(0, USignature, "%S", alg)

      // OpenSSL_add_all_algorithms();

      state          = UNINITIALIZED;
      context.digest = getEVP_MD(alg);
      }

   /**
   * Deletes this object.
   */

   ~USignature()
      {
      U_TRACE_UNREGISTER_OBJECT(0, USignature)
      }

   /**
   * Returns a string that identifies the algorithm, independent of
   * implementation details. The name should be a standard
   * name such as "RSA-MD5" or "DSA-SHA1"
   *
   * @return name of the algorithm
   */

   const char* getAlgorithm() const { return algorithm; }

   /**
   * Resets the digest engine.
   */

   void resetEngine()
      {
      U_TRACE(1, "USignature::resetEngine()")

      U_SYSCALL_VOID(EVP_DigestInit, "%p,%p", &context, context.digest);
      }

   /**
   * Initializes this object for signing. If this method is called
   * again with a different argument, it negates the effect
   * of this call.
   *
   * @param privateKey the private key of the identity whose signature is going to be generated
   * @see resetEngine()
   */

   void initSign(EVP_PKEY* privKey)
      {
      U_TRACE(0, "USignature::initSign(%p)", privKey)

      state      = SIGN;
      privateKey = privKey;

      resetEngine();
      }

   /**
   * Initializes this object for verification. If this method is called
   * again with a different argument, it negates the effect of this call.
   *
   * @param publicKey the public key of the identity whose signature is going to be verified
   * @see resetEngine()
   */

   void initVerify(EVP_PKEY* pubKey)
      {
      U_TRACE(0, "USignature::initVerify(%S)", pubKey)

      state     = VERIFY;
      publicKey = pubKey;

      resetEngine();
      }

   /**
   * Updates the data to be signed or verified.
   *
   * @param data the string of bytes to use for the update
   * @param offset start index
   * @param len the number of bytes to use starting at <i>offset</i>
   */

   void update(const char* data, int offset = 0, int len = 0)
      {
      U_TRACE(1, "USignature::update(%.*S,%d,%d)", len, data, offset, len)

      if (len == 0) len = strlen(data);

      U_SYSCALL_VOID(EVP_DigestUpdate, "%p,%p,%d", &context, data + offset, len);
      }

   /**
   * Updates the data to be signed or verified.
   *
   * @param data the string of bytes to use for the update.
   * @see update(const char*, int, int)
   */

   void update(const UString& data) { update(data.data(), 0, data.size()); }

   /**
   * Returns the digital signature of whatever data has been
   * accumlated from the update calls.
   *
   * Error if <i>initSign</i> has not been called or if the private key is uninitialized
   */

   UString sign();
   UString sign(const UString& data)                           { update(data);              return sign(); }
   UString sign(const char* data, int offset = 0, int len = 0) { update(data, offset, len); return sign(); }

   /**
   * Returns true if signature verifies, false otherwise. All the data
   * that has been accumulated by update calls is signed by the public
   * key and the results are compared to the input signature. If they
   * match verify is true.
   *
   * Error if <i>initVerify</i> has not been called or if the public key is uninitialized
   *
   * @param signature digital signature to be verified
   */

   bool verify(const UString& signature)
      {
      U_TRACE(0, "USignature::verify(%.*S)", U_STRING_TO_TRACE(signature))

      U_INTERNAL_ASSERT_EQUALS(state,VERIFY)

      bool result = (EVP_VerifyFinal(&context, (unsigned char*)signature.data(), signature.size(), publicKey) == 1);

      U_RETURN(result);
      }

#ifdef DEBUG
   const char* dump(bool reset) const;
#endif

protected:
   EVP_MD_CTX context;
   const char* algorithm;

   EVP_PKEY* publicKey;  // used for verifying
   EVP_PKEY* privateKey; // used for signing

   enum State { UNINITIALIZED, SIGN, VERIFY } state;

private:
   USignature(const USignature&)            {}
   USignature& operator=(const USignature&) { return *this; }
};

#endif

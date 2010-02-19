// sign.cpp

#include <ulib/url.h>
#include <ulib/date.h>
#include <ulib/file_config.h>
#include <ulib/base/ssl/dgst.h>
#include <ulib/utility/base64.h>
#include <ulib/ssl/certificate.h>
#include <ulib/utility/services.h>
#include <ulib/utility/xml_escape.h>
#include <ulib/utility/string_ext.h>
#include <ulib/xml/libxml2/document.h>

#ifdef HAVE_SSL_TS
#  include <ulib/ssl/timestamp.h>
#endif

#undef  PACKAGE
#define PACKAGE "sign"

#define ARGS "[file1 file2 file3...]"

#define U_OPTIONS \
"purpose \"simple XAdES signature...\"\n" \
"option c config      1 \"path of configuration file\" \"\"\n" \
"option C CApath      1 \"path for certificates verification\" \"./CApath\"\n" \
"option k key         1 \"encrypted pkcs-8 private key file\" \"\"\n" \
"option p password    1 \"password for encrypted pkcs-8 private key file\" \"\"\n" \
"option X certificate 1 \"signer's X.509 certificate file that matches the private key\" \"\"\n"

#include <ulib/application.h>

//#define XMLDSIG_ONLY

#define U_XMLDSIG_REFERENCE_TEMPLATE \
"    <ds:Reference URI=\"#ObjID_%u\">\r\n" \
"      <ds:DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#%.*s\"></ds:DigestMethod>\r\n" \
"      <ds:DigestValue>%.*s</ds:DigestValue>\r\n" \
"    </ds:Reference>\r\n"

#define U_XMLDSIG_SIGNED_INFO_TEMPLATE \
"  <ds:SignedInfo xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\">\r\n" \
"    <ds:CanonicalizationMethod Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315\"></ds:CanonicalizationMethod>\r\n" \
"    <ds:SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-%.*s\"></ds:SignatureMethod>\r\n" \
"%.*s" \
"%.*s" \
"  </ds:SignedInfo>\r\n"

#define U_XMLDSIG_SIGNATURE_VALUE_TEMPLATE \
"  <ds:SignatureValue xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\">\r\n" \
"%.*s" \
"  </ds:SignatureValue>\r\n" \

#define U_XMLDSIG_OBJECT_TEMPLATE \
"  <ds:Object xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" Id=\"ObjID_%u\">%.*s</ds:Object>\r\n"

#define U_XMLDSIG_TEMPLATE \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" \
"<ds:Signature xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" Id=\"SigID\">\r\n" \
"%.*s" \
"%.*s" \
"  <ds:KeyInfo>\r\n" \
"    <ds:KeyValue>\r\n" \
"      <ds:RSAKeyValue>\r\n" \
"        <ds:Modulus>\r\n" \
"%.*s" \
"        </ds:Modulus>\r\n" \
"        <ds:Exponent>%.*s</ds:Exponent>\r\n" \
"      </ds:RSAKeyValue>\r\n" \
"    </ds:KeyValue>\r\n" \
"    <ds:X509Data>\r\n" \
"      <ds:X509SubjectName>\r\n" \
"        %.*s\r\n" \
"      </ds:X509SubjectName>\r\n" \
"      <ds:X509IssuerSerial>\r\n" \
"        <ds:X509IssuerName>\r\n" \
"          %.*s\r\n" \
"        </ds:X509IssuerName>\r\n" \
"        <ds:X509SerialNumber>%ld</ds:X509SerialNumber>\r\n" \
"      </ds:X509IssuerSerial>\r\n" \
"      <ds:X509Certificate>\r\n" \
"%.*s" \
"      </ds:X509Certificate>\r\n" \
"    </ds:X509Data>\r\n" \
"  </ds:KeyInfo>\r\n" \
"%.*s" \
"%.*s" \
"</ds:Signature>\r\n"

#define U_XADES_REFERENCE_TEMPLATE \
"    <ds:Reference URI=\"#SigID-SignedProperties\" Type=\"http://uri.etsi.org/01903#SignedProperties\">\r\n" \
"      <ds:DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#%.*s\"></ds:DigestMethod>\r\n" \
"      <ds:DigestValue>%.*s</ds:DigestValue>\r\n" \
"    </ds:Reference>\r\n"

#define U_XADES_SIGNING_TIME_TEMPLATE \
"          <xades:SigningTime>%.*s</xades:SigningTime>\r\n"

#define U_XADES_SIGNER_ROLE_TEMPLATE \
"          <xades:SignerRole>\r\n" \
"            <xades:ClaimedRoles>\r\n" \
"              <xades:ClaimedRole>%.*s</xades:ClaimedRole>\r\n" \
"            </xades:ClaimedRoles>\r\n" \
"          </xades:SignerRole>\r\n"

#define U_XADES_DATA_OBJECT_FORMAT_TEMPLATE \
"         <xades:DataObjectFormat ObjectReference=\"#ObjID_%u\">\r\n" \
"           <xades:MimeType>%.*s</xades:MimeType>\r\n" \
"         </xades:DataObjectFormat>\r\n"

#define U_XADES_ALL_DATA_OBJECTS_TIMESTAMP_TEMPLATE \
"         <xades:AllDataObjectsTimeStamp>\r\n" \
"           <xades:EncapsulatedTimeStamp>\r\n" \
"%.*s" \
"           </xades:EncapsulatedTimeStamp>\r\n" \
"         </xades:AllDataObjectsTimeStamp>\r\n"

#define U_XADES_CERTIFICATE_TEMPLATE \
"            <xades:Cert>\r\n" \
"              <xades:CertDigest>\r\n" \
"                <ds:DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#%.*s\"></ds:DigestMethod>\r\n" \
"                <ds:DigestValue>%.*s</ds:DigestValue>\r\n" \
"              </xades:CertDigest>\r\n" \
"              <xades:IssuerSerial>\r\n" \
"                <ds:X509IssuerName>\r\n" \
"                  %.*s\r\n" \
"                </ds:X509IssuerName>\r\n" \
"                <ds:X509SerialNumber>%ld</ds:X509SerialNumber>\r\n" \
"              </xades:IssuerSerial>\r\n" \
"            </xades:Cert>\r\n"

#define U_XADES_CRL_TEMPLATE \
"            <xades:CRLRefs>\r\n" \
"              <xades:CRLRef">\r\n" \
"                <xades:DigestAlgAndValue>\r\n" \
"                  <ds:DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#%.*s\"></ds:DigestMethod>\r\n" \
"                  <ds:DigestValue>%.*s</ds:DigestValue>\r\n"

                  /*
                  <OCSPIdentifier URI="#N0">
                    <ResponderID>/C=EE/O=ESTEID/OU=OCSP/CN=ESTEID-SK OCSP RESPONDER/emailAddress=pki@sk.ee</ResponderID>
                    <ProducedAt>2003.04.22T10:32:46Z</ProducedAt>
                  </OCSPIdentifier>
                  <DigestAlgAndValue>
                    <DigestMethod Algorithm="http://www.w3.org/2000/09/xmldsig#sha1"></DigestMethod>
                    <DigestValue>UoUfe+XSnc7kUKKkdQvpBV4+pBA=</DigestValue>
                  </DigestAlgAndValue>
                </OCSPRef>
              </OCSPRefs>
               */

#define U_XADES_ENCAPSULATED_X509_CERTIFICATE_TEMPLATE \
"            <xades:EncapsulatedX509Certificate>\r\n" \
"%.*s" \
"            </xades:EncapsulatedX509Certificate>\r\n"

#define U_XADES_ENCAPSULATED_CRL_VALUE_TEMPLATE \
"            <xades:EncapsulatedCRLValue">\r\n" \
"%.*s" \
"            </xades:EncapsulatedCRLValue">\r\n"

#define U_XADES_SIGNED_PROPERTIES_TEMPLATE \
"      <xades:SignedProperties xmlns:xades=\"http://uri.etsi.org/01903/v1.4.1#\" xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" Id=\"SigID-SignedProperties\">\r\n" \
"        <xades:SignedSignatureProperties>\r\n" \
"%.*s" \
"          <xades:SigningCertificate>\r\n" \
"%.*s" \
"          </xades:SigningCertificate>\r\n" \
"          <xades:SignaturePolicyIdentifier>\r\n" \
"            <xades:SignaturePolicyImplied></xades:SignaturePolicyImplied>\r\n" \
"          </xades:SignaturePolicyIdentifier>\r\n" \
"          <xades:SignatureProductionPlace>\r\n" \
"            <xades:City>%.*s</xades:City>\r\n" \
"            <xades:StateOrProvince>%.*s</xades:StateOrProvince>\r\n" \
"            <xades:PostalCode>%.*s</xades:PostalCode>\r\n" \
"            <xades:CountryName>%.*s</xades:CountryName>\r\n" \
"          </xades:SignatureProductionPlace>\r\n" \
"%.*s" \
"        </xades:SignedSignatureProperties>\r\n" \
"        <xades:SignedDataObjectProperties>\r\n" \
"%.*s" \
"%.*s" \
"        </xades:SignedDataObjectProperties>\r\n" \
"      </xades:SignedProperties>\r\n"

#define U_XADES_SIGNATURE_TIMESTAMP_TEMPLATE \
"          <xades:SignatureTimeStamp>\r\n" \
"%.*s" \
"          </xades:SignatureTimeStamp>\r\n"

// RefsOnlyTimeStamp: it contains a time-stamp token only over all certificate and revocation
//                    information references.

#define U_XADES_REFSONLY_TIMESTAMP_TEMPLATE \
"          <xades:RefsOnlyTimeStamp>\r\n" \
"%.*s" \
"          </xades:RefsOnlyTimeStamp>\r\n"

// SigAndRefsTimeStamp: it contains a time-stamp token computed over the signature value, the
//                      signature time-stamp and the certificate and revocation information references.

#define U_XADES_SIGANDREFS_TIMESTAMP_TEMPLATE \
"          <xades:SigAndRefsTimeStamp>\r\n" \
"%.*s" \
"          </xades:SigAndRefsTimeStamp>\r\n"

// ArchiveTimeStamp

#define U_XADES_ARCHIVE_TIMESTAMP_TEMPLATE \
"          <xadesv141:ArchiveTimeStamp>\r\n" \
"%.*s" \
"          </xadesv141:ArchiveTimeStamp>\r\n"

#define U_XADES_COMPLETE_CERTIFICATE_REFS_TEMPLATE \
"          <xades:CompleteCertificateRefs>\r\n" \
"%.*s" \
"          </xades:CompleteCertificateRefs>\r\n"

#define U_XADES_COMPLETE_REVOCATION_REFS_TEMPLATE \
"          <xades:CompleteRevocationRefs>\r\n" \
"%.*s" \
"          </xades:CompleteRevocationRefs>\r\n"

#define U_XADES_CERTIFICATE_VALUES_TEMPLATE \
"          <xades:CertificateValues>\r\n" \
"%.*s" \
"          </xades:CertificateValues>\r\n"

#define U_XADES_REVOCATION_VALUES_TEMPLATE \
"          <xades:RevocationValues>\r\n" \
"%.*s" \
"          </xades:RevocationValues>\r\n"

#define U_XADES_UNSIGNED_SIGNATURE_PROPERTIES_TEMPLATE \
"%.*s" \
"%.*s" \
"%.*s" \
"%.*s"

#define U_XADES_TEMPLATE \
"  <ds:Object xmlns:xades=\"http://uri.etsi.org/01903/v1.4.1#\" xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\">\r\n" \
"    <xades:QualifyingProperties Target=\"#SigID\">\r\n" \
"%.*s" \
"      <xades:UnsignedProperties>\r\n" \
"        <xades:UnsignedSignatureProperties>\r\n" \
"%.*s" \
"%.*s" \
"        </xades:UnsignedSignatureProperties>\r\n" \
"      </xades:UnsignedProperties>\r\n" \
"    </xades:QualifyingProperties>\r\n" \
"  </ds:Object>\r\n"

/* XAdES-C

XML Advanced Electronic Signature with Complete validation data references (XAdES-C)
in accordance with the present document adds to the XAdES-T the CompleteCertificateRefs
and CompleteRevocationRefs unsigned properties as defined by the present document.
If attribute certificates appear in the signature, then XAdES-C also incorporates the
AttributeCertificateRefs and the AttributeRevocationRefs elements. CompleteCertificateRefs
element contains a sequence of references to the full set of CA certificates that have been
used to validate the electronic signature up to (but not including) the signing certificate.
CompleteRevocationRefs element contains a full set of references to the revocation data that
have been used in the validation of the signer and CA certificates. AttributeCertificateRefs
and AttributeRevocationRefs elements contain references to the full set of Attribute Authorities
certificates and references to the full set of revocation data that have been used in the validation
of the attribute certificates present in the signature, respectively. Storing the references allows
the values of the certification path and revocation data to be stored elsewhere, reducing the size
of a stored electronic signature format. 
*/

#define U_BASE64_MAX_COLUMN 64

/*
XMLDSIG of simple text string.
-------------------------------------------------------------------------------------------------------------------
INPUT: T    text-to-be-signed, a byte string
       Ks   RSA private key

OUTPUT: XML file

   1. Canonicalize (strictly, what we are doing here is encapsulating the text string T inside an <Object> element,
      then canonicalizing that element) the text-to-be-signed, C = C14n(T).
   2. Compute the message digest of the canonicalized text, m = Hash(C).
   3. Encapsulate the message digest in an XML <SignedInfo> element, SI, in canonicalized form.
   4. Compute the RSA signatureValue of the canonicalized <SignedInfo> element, SV = RsaSign(Ks, SI).
   5. Compose the final XML document including the signatureValue, this time in non-canonicalized form.
-------------------------------------------------------------------------------------------------------------------
*/

class Application : public UApplication {
public:

   ~Application()
      {
      U_TRACE(5, "Application::~Application()")
      }

   UString getTimeStampToken(const UString& data, const UString& url)
      {
      U_TRACE(5, "Application::getTimeStampToken(%.*S,%.*S)", U_STRING_TO_TRACE(data), U_STRING_TO_TRACE(url))

      U_ASSERT(url.empty() == false)

      Url TSA(url);
      UString token;

#  ifdef HAVE_SSL_TS
      UString request = UTimeStamp::createQuery(alg, data, 0, false, false);

      UTimeStamp ts(request, TSA);

      if (ts.UPKCS7::isValid()) token = ts.UPKCS7::getEncoded("BASE64", U_BASE64_MAX_COLUMN);
#  endif

      U_RETURN_STRING(token);
      }

   void setXAdES()
      {
      U_TRACE(5, "Application::setXAdES()")

      // SIGNED PROPERTIES

      UString signingCertificate(U_CAPACITY);

      // Compute the digest of the signer certificate

      X509CertificateValue.reserve(X509Certificate.size() * 4);

      UBase64::encode(X509Certificate, X509CertificateValue, U_BASE64_MAX_COLUMN);

      UString X509CertificateDigestValue(U_CAPACITY);

      UServices::generateDigest(alg, 0, X509Certificate, X509CertificateDigestValue, true, 0);

      signingCertificate.snprintf(U_XADES_CERTIFICATE_TEMPLATE,
                                  U_STRING_TO_TRACE(digest_algorithm),
                                  U_STRING_TO_TRACE(X509CertificateDigestValue),
                                  U_STRING_TO_TRACE(X509IssuerName),
                                  X509SerialNumber);

      UString signingTime(100U);

      if (signing_time)
         {
         UString dateTime = UDate::strftime("%Y-%m-%dT%H:%M:%SZ", 0);

         signingTime.snprintf(U_XADES_SIGNING_TIME_TEMPLATE, U_STRING_TO_TRACE(dateTime));
         }

      UString roleTemplate(U_CAPACITY);

      if (claimed_role.empty() == false) roleTemplate.snprintf(U_XADES_SIGNER_ROLE_TEMPLATE, U_STRING_TO_TRACE(claimed_role));

      UString allDataObjectTimestamp(U_CAPACITY);

      if (all_data_object_timestamp.empty() == false)
         {
         UString token = getTimeStampToken(all_data_object, all_data_object_timestamp);

         allDataObjectTimestamp.snprintf(U_XADES_ALL_DATA_OBJECTS_TIMESTAMP_TEMPLATE, U_STRING_TO_TRACE(token));
         }

      signedProperties.reserve(U_CONSTANT_SIZE(U_XADES_SIGNED_PROPERTIES_TEMPLATE) + 8192U +
                               signingTime.size() + allDataObjectTimestamp.size());

      signedProperties.snprintf(U_XADES_SIGNED_PROPERTIES_TEMPLATE,
                                U_STRING_TO_TRACE(signingTime),
                                U_STRING_TO_TRACE(signingCertificate),
                                U_STRING_TO_TRACE(production_place_city),
                                U_STRING_TO_TRACE(production_place_state_or_province),
                                U_STRING_TO_TRACE(production_place_postal_code),
                                U_STRING_TO_TRACE(production_place_country_name),
                                U_STRING_TO_TRACE(roleTemplate),
                                U_STRING_TO_TRACE(DataObjectFormat),
                                U_STRING_TO_TRACE(allDataObjectTimestamp));

      to_digest = UXML2Document(signedProperties).xmlC14N();

      UString signedPropertiesDigestValue(200U);

      UServices::generateDigest(alg, 0, to_digest, signedPropertiesDigestValue, true, 0);

      XAdESReference.reserve(U_CONSTANT_SIZE(U_XADES_REFERENCE_TEMPLATE) + signedPropertiesDigestValue.size());

      XAdESReference.snprintf(U_XADES_REFERENCE_TEMPLATE,
                              U_STRING_TO_TRACE(digest_algorithm),
                              U_STRING_TO_TRACE(signedPropertiesDigestValue));

      // UNSIGNED SIGNATURE PROPERTIES

      UString completeCertificateRef(U_CAPACITY), completeCertificateRefs(U_CAPACITY);

      uint32_t i, n;
      long CASerialNumber;
      UVector<UString> vec_CACertificateValue;
      UString item(U_CAPACITY), CACertificateValue(U_CAPACITY), CAIssuerName, CACertificate;

      UCertificate* ca;

      for (i = 0; i < num_ca; ++i)
         {
         ca = vec_ca[i];

         CAIssuerName   = ca->getIssuerForLDAP();
         CACertificate  = ca->getEncoded("DER");
         CASerialNumber = ca->getSerialNumber();

         UBase64::encode(CACertificate, CACertificateValue, U_BASE64_MAX_COLUMN);

         vec_CACertificateValue.push_back(CACertificateValue);

         UServices::generateDigest(alg, 0, CACertificate, X509CertificateDigestValue, true, 0);

         item.snprintf(U_XADES_CERTIFICATE_TEMPLATE,
                       U_STRING_TO_TRACE(digest_algorithm),
                       U_STRING_TO_TRACE(X509CertificateDigestValue),
                       U_STRING_TO_TRACE(CAIssuerName),
                       CASerialNumber);

         (void) completeCertificateRef.append(item);
         }

      completeCertificateRefs.snprintf(U_XADES_COMPLETE_CERTIFICATE_REFS_TEMPLATE, U_STRING_TO_TRACE(completeCertificateRef));

      UString certificateValue(U_CAPACITY), certificateValues(U_CAPACITY);

      for (i = 0, n = vec_CACertificateValue.size(); i < n; ++i)
         {
         CACertificateValue = vec_CACertificateValue[i];

         item.snprintf(U_XADES_ENCAPSULATED_X509_CERTIFICATE_TEMPLATE,
                       U_STRING_TO_TRACE(CACertificateValue));

         (void) certificateValue.append(item);
         }

      certificateValues.snprintf(U_XADES_CERTIFICATE_VALUES_TEMPLATE, U_STRING_TO_TRACE(certificateValue));

      UString completeRevocationRefs(U_CAPACITY);

      completeRevocationRefs.snprintf(U_XADES_COMPLETE_REVOCATION_REFS_TEMPLATE, U_STRING_TO_TRACE(completeRevocationRefs));


      UString revocationValues(U_CAPACITY);

      revocationValues.snprintf(U_XADES_REVOCATION_VALUES_TEMPLATE, U_STRING_TO_TRACE(revocationValues));

      unsignedSignatureProperties.reserve(U_CONSTANT_SIZE(U_XADES_UNSIGNED_SIGNATURE_PROPERTIES_TEMPLATE) +
                                          completeCertificateRefs.size() +
                                          completeRevocationRefs.size() +
                                          certificateValues.size() +
                                          revocationValues.size());

      unsignedSignatureProperties.snprintf(U_XADES_UNSIGNED_SIGNATURE_PROPERTIES_TEMPLATE,
                                           U_STRING_TO_TRACE(completeCertificateRefs),
                                           U_STRING_TO_TRACE(certificateValues),
                                           U_STRING_TO_TRACE(completeRevocationRefs),
                                           U_STRING_TO_TRACE(revocationValues));
      }

   void run(int argc, char* argv[], char* env[])
      {
      U_TRACE(5, "Application::run(%d,%p,%p)", argc, argv, env)

      UApplication::run(argc, argv, env);

      // manage options

      if (UApplication::isOptions())
         {
         passwd     = opt['p'];
         cfg_str    = opt['c'];
         key_str    = opt['k'];
         crt_str    = opt['X'];
         str_CApath = opt['C'];
         }

      if (key_str.empty() ||
          crt_str.empty() ||
          str_CApath.empty())
         {
         usage();
         }

      UString x = UFile::contentOf(key_str);

      if (x.empty() ||
          (u_pkey = UServices::loadKey(x, 0, true, passwd.c_str(), 0)) == 0)
         {
         U_ERROR("I can't load the private key: %.*s", U_STRING_TO_TRACE(key_str));
         }

      x = UFile::contentOf(crt_str);

      if (x.empty()) U_ERROR("I can't load the certificate: %.*s", U_STRING_TO_TRACE(crt_str));

      UCertificate cert(x);

#  ifdef HAVE_OPENSSL_98
      if (cert.matchPrivateKey(u_pkey) == false) U_ERROR("the private key doesn't matches the public key of the certificate", 0);
#  endif

      if (str_CApath.empty() ||
          UServices::setupOpenSSLStore(0, str_CApath.c_str()) == false)
         {
         U_ERROR("error on setting CApath: %S", str_CApath.data());
         }

      num_ca = cert.getSignerCertificates(vec_ca, 0, 0);

      if (UCertificate::verify_result == false)
         {
         U_ERROR("error on verifying the certificate: %.*s - %s", U_STRING_TO_TRACE(crt_str), UServices::verify_status());
         }

      // manage file configuration

      if (cfg_str.empty()) cfg_str = U_STRING_FROM_CONSTANT("sign.cfg");

      // ----------------------------------------------------------------------------------------------------------------------------------
      // XAdES signature - configuration parameters
      // ----------------------------------------------------------------------------------------------------------------------------------
      // DIGEST_ALGORITHM  md2 | md5 | sha | sha1 | sha224 | sha256 | sha384 | sha512 | mdc2 | ripmed160
      //
      // SIGNING_TIME   this property contains the time at which the signer claims to have performed the signing process (yes/no)
      // CLAIMED_ROLE   This property contains claimed or certified roles assumed by the signer in creating the signature
      //
      // this property contains the indication of the purported place where the signer claims to have produced the signature
      // -------------------------------------------------------------------------------------------------------------------
      // PRODUCTION_PLACE_CITY
      // PRODUCTION_PLACE_STATE_OR_PROVINCE
      // PRODUCTION_PLACE_POSTAL_CODE
      // PRODUCTION_PLACE_COUNTRY_NAME
      // -------------------------------------------------------------------------------------------------------------------
      //
      // DATA_OBJECT_FORMAT_MIMETYPE   this property identifies the format of a signed data object (when electronic signatures
      //                               are not exchanged in a restricted context) to enable the presentation to the verifier or
      //                               use by the verifier (text, sound or video) in exactly the same way as intended by the signer
      //
      // SIGNATURE_TIMESTAMP           the time-stamp token within this property covers the digital signature value element
      // ALL_DATA_OBJECT_TIMESTAMP     the time-stamp token within this property covers all the signed data object
      // ----------------------------------------------------------------------------------------------------------------------------------

      (void) cfg.open(cfg_str);

      signing_time                        = cfg.readBoolean(U_STRING_FROM_CONSTANT("SIGNING_TIME"));

      claimed_role                        = cfg[U_STRING_FROM_CONSTANT("CLAIMED_ROLE")];
      digest_algorithm                    = cfg[U_STRING_FROM_CONSTANT("DIGEST_ALGORITHM")];
      signature_timestamp                 = cfg[U_STRING_FROM_CONSTANT("SIGNATURE_TIMESTAMP")];
      all_data_object_timestamp           = cfg[U_STRING_FROM_CONSTANT("ALL_DATA_OBJECT_TIMESTAMP")];
      data_object_format_mimetype         = cfg[U_STRING_FROM_CONSTANT("DATA_OBJECT_FORMAT_MIMETYPE")];

      production_place_city               = cfg[U_STRING_FROM_CONSTANT("PRODUCTION_PLACE_CITY")];
      production_place_state_or_province  = cfg[U_STRING_FROM_CONSTANT("PRODUCTION_PLACE_STATE_OR_PROVINCE")];
      production_place_postal_code        = cfg[U_STRING_FROM_CONSTANT("PRODUCTION_PLACE_POSTAL_CODE")];
      production_place_country_name       = cfg[U_STRING_FROM_CONSTANT("PRODUCTION_PLACE_COUNTRY_NAME")];

      if (digest_algorithm.empty()) digest_algorithm = U_STRING_FROM_CONSTANT("sha1");

      alg = u_dgst_get_algoritm(digest_algorithm.c_str());

      if (alg == -1) U_ERROR("I can't find the digest algorithm for: %s", digest_algorithm.data());

      u_line_terminator_len = 2;

      UString modulus          = cert.getModulus(U_BASE64_MAX_COLUMN),
              exponent         = cert.getExponent(U_BASE64_MAX_COLUMN);
              X509IssuerName   = cert.getIssuerForLDAP(),
              X509SubjectName  = cert.getSubjectForLDAP(),
              X509Certificate  = cert.getEncoded("DER");
              X509SerialNumber = cert.getSerialNumber();

      // manage arg operation

      const char* file;
      UString XMLDSIGObject(U_CAPACITY), Object(U_CAPACITY), ObjectDigestValue(200U),
              document_to_add, Reference(U_CAPACITY), dataObjectFormat(U_CAPACITY), XMLDSIGReference(U_CAPACITY);

      for (uint32_t i = 1; (file = argv[optind]); ++i, ++optind)
         {
         document = UFile::contentOf(file);

         if (document.empty() || document.isBinary()) U_ERROR("I can't sign this document: %s", file);

         if (u_endsWith(file, strlen(file), U_CONSTANT_TO_PARAM(".xml")) == false) document_to_add = document;
         else
            {
            document_to_add = UXML2Document(document).xmlC14N();

            UString tmp(document.size() * 4);

            UXMLEscape::encode(document, tmp);

            document = tmp;
            }

         all_data_object += document_to_add;

         document = UStringExt::dos2unix(document, true);

         // ---------------------------------------------------------------------------------------------------------------
         // 1. Canonicalize (strictly, what we are doing here is encapsulating the text string T inside an <Object> element,
         //    then canonicalizing that element) the text-to-be-signed, C = C14n(T).
         // ---------------------------------------------------------------------------------------------------------------
         Object.reserve(U_CONSTANT_SIZE(U_XMLDSIG_OBJECT_TEMPLATE) + document.size());

         Object.snprintf(U_XMLDSIG_OBJECT_TEMPLATE, i, U_STRING_TO_TRACE(document));

         XMLDSIGObject += Object;

         to_digest = UXML2Document(Object).xmlC14N();
         // ---------------------------------------------------------------------------------------------------------------

         // ---------------------------------------------------------------------------------------------------------------
         // 2. Compute the message digest of the canonicalized text, m = Hash(C).
         // ---------------------------------------------------------------------------------------------------------------
         ObjectDigestValue.setEmpty();

         UServices::generateDigest(alg, 0, to_digest, ObjectDigestValue, true, 0);
         // ---------------------------------------------------------------------------------------------------------------

         Reference.snprintf(U_XMLDSIG_REFERENCE_TEMPLATE, i,
                              U_STRING_TO_TRACE(digest_algorithm),
                              U_STRING_TO_TRACE(ObjectDigestValue));

         XMLDSIGReference += Reference;

         if (data_object_format_mimetype.empty() == false)
            {
            dataObjectFormat.snprintf(U_XADES_DATA_OBJECT_FORMAT_TEMPLATE, i, U_STRING_TO_TRACE(data_object_format_mimetype));

            DataObjectFormat += dataObjectFormat;
            }
         }

      setXAdES(); // XAdES management

      // ---------------------------------------------------------------------------------------------------------------
      // 3. Encapsulate the message digest in an XML <SignedInfo> element, SI, in canonicalized form.
      // ---------------------------------------------------------------------------------------------------------------
      UString SignedInfo(U_CONSTANT_SIZE(U_XMLDSIG_SIGNED_INFO_TEMPLATE) + XMLDSIGReference.size() + XAdESReference.size());

#  ifdef XMLDSIG_ONLY
      XAdESReference.clear();
#  endif

      SignedInfo.snprintf(U_XMLDSIG_SIGNED_INFO_TEMPLATE,
                          U_STRING_TO_TRACE(digest_algorithm),
                          U_STRING_TO_TRACE(XMLDSIGReference),
                          U_STRING_TO_TRACE(XAdESReference));

      UString to_sign = UXML2Document(SignedInfo).xmlC14N();
      // ---------------------------------------------------------------------------------------------------------------

      // ---------------------------------------------------------------------------------------------------------------
      // 4. Compute the RSA signatureValue of the canonicalized <SignedInfo> element, SV = RsaSign(Ks, SI).
      // ---------------------------------------------------------------------------------------------------------------
      UString SignatureValue(U_CAPACITY), signatureTimestamp(U_CAPACITY);

      UString sign = UServices::getSignatureValue(alg, to_sign, UString::getStringNull(), UString::getStringNull(), true, U_BASE64_MAX_COLUMN);

      SignatureValue.snprintf(U_XMLDSIG_SIGNATURE_VALUE_TEMPLATE, U_STRING_TO_TRACE(sign));

      if (signature_timestamp.empty() == false)
         {
         to_digest = UXML2Document(SignatureValue).xmlC14N();

         UString token = getTimeStampToken(to_digest, signature_timestamp);

         signatureTimestamp.snprintf(U_XADES_SIGNATURE_TIMESTAMP_TEMPLATE, U_STRING_TO_TRACE(token));
         }

      XAdESObject.reserve(U_CONSTANT_SIZE(U_XADES_TEMPLATE) +
                          signedProperties.size() +
                          unsignedSignatureProperties.size() +
                          signatureTimestamp.size());

      XAdESObject.snprintf(U_XADES_TEMPLATE,
                           U_STRING_TO_TRACE(signedProperties),
                           U_STRING_TO_TRACE(unsignedSignatureProperties),
                           U_STRING_TO_TRACE(signatureTimestamp));
      // ---------------------------------------------------------------------------------------------------------------

      // ---------------------------------------------------------------------------------------------------------------
      // 5. Compose the final XML document including the signatureValue, this time in non-canonicalized form.
      // ---------------------------------------------------------------------------------------------------------------
      UString output(U_CONSTANT_SIZE(U_XMLDSIG_TEMPLATE) + 8192U + 
                     SignedInfo.size() + SignatureValue.size() + XMLDSIGObject.size() + XAdESObject.size());

#  ifdef XMLDSIG_ONLY
      XAdESObject.clear();
#  endif

      output.snprintf(U_XMLDSIG_TEMPLATE,
                        U_STRING_TO_TRACE(SignedInfo),
                        U_STRING_TO_TRACE(SignatureValue),
                        U_STRING_TO_TRACE(modulus),
                        U_STRING_TO_TRACE(exponent),
                        U_STRING_TO_TRACE(X509SubjectName),
                        U_STRING_TO_TRACE(X509IssuerName),
                        X509SerialNumber,
                        U_STRING_TO_TRACE(X509CertificateValue),
                        U_STRING_TO_TRACE(XMLDSIGObject),
                        U_STRING_TO_TRACE(XAdESObject));
      // ---------------------------------------------------------------------------------------------------------------

      cout.write(U_STRING_TO_PARAM(output));
      }

private:
   int alg;
   uint32_t num_ca;
   UFileConfig cfg;
   bool signing_time;
   long X509SerialNumber;
   UVector<UCertificate*> vec_ca;
   UString cfg_str, key_str, crt_str, digest_algorithm, canonicalization_algorithm, claimed_role,
           production_place_city, production_place_state_or_province, production_place_postal_code,
           production_place_country_name, data_object_format_mimetype, signature_algorithm, document,
           passwd, to_digest, DataObjectFormat, XAdESObject, XAdESReference, X509IssuerName, X509SubjectName,
           X509Certificate, X509CertificateValue, signedProperties, all_data_object, all_data_object_timestamp,
           signature_timestamp, str_CApath, unsignedSignatureProperties;
};

U_MAIN(Application)

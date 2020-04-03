// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Manager
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography.Xml;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using Security;
    using Security.Cryptography;
    
    /// <summary>
    /// class to sign the xml files and verify the signed files.
    /// </summary>
    public static class XmlCryptography
    {
        /// <summary>
        /// Specifies the key container name for Xml Digital signature.
        /// </summary>
        private const string XmlDsigKeyContainerName = "XML_DSIG_RSA_KEY";

        /// <summary>
        /// Specifies the PROV_RSA_AET provider type.
        /// </summary>
        private const int ProviderRsaAetType = 24;

        /// <summary>
        /// Specifies a URI that points to the RSA-SHA256 cryptographic algorithm for digitally signing XML.
        /// </summary>
        private const string XmlSha256DigestMethod = "http://www.w3.org/2001/04/xmlenc#sha256";

        /// <summary>
        /// Specifies a URI that points to the RSA-SHA256 cryptographic algorithm for digitally signing XML.
        /// </summary>
        private const string XmlDsigSha256SignatureMethod = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";

        /// <summary>
        /// Password used for opening certificate file.
        /// </summary>
        private static string password = string.Empty;

        /// <summary>
        /// The Cryptographic service provider that contains the private key.
        /// </summary>
        private static string cspName = string.Empty;

        /// <summary>
        /// The Private Key Container for the CSP
        /// </summary>
        private static string keyContainer = string.Empty;

        /// <summary>
        /// verified the xml signature with the help of x.509 certificate.
        /// </summary>
        /// <param name="xmlDoc">XML document to be verified</param>
        /// <param name="key"> Certificate's subject</param>
        /// <returns>true or false</returns>
        public static bool IsXmlSignValid(XmlDocument xmlDoc, string key)
        {            
            CryptoConfig.AddAlgorithm(typeof(Security.Cryptography.RSAPKCS1SHA256SignatureDescription), "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
            CspParameters cspParams = new CspParameters();
            cspParams.KeyContainerName = "XML_DSIG_RSA_KEY";
            RSACryptoServiceProvider csp = new RSACryptoServiceProvider(cspParams);
    
            // loading the key string
            csp.FromXmlString(key);
            xmlDoc.PreserveWhitespace = true;
            
            SignedXml signedXml = new SignedXml(xmlDoc);
            signedXml.SignedInfo.CanonicalizationMethod = SignedXml.XmlDsigCanonicalizationUrl;
            signedXml.SignedInfo.SignatureMethod = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";
            signedXml.SigningKey = csp;

            // Create a reference to be signed.
            Reference reference = new Reference(string.Empty);
            reference.DigestMethod = "http://www.w3.org/2001/04/xmlenc#sha256";

            // Add an enveloped transformation to the reference.
            XmlDsigEnvelopedSignatureTransform env = new XmlDsigEnvelopedSignatureTransform(false);
            reference.AddTransform(env);
            signedXml.AddReference(reference);

            // Find the "Signature" node and create a new 
            XmlNodeList nodeList = xmlDoc.GetElementsByTagName("Signature");

            // Load the first <signature> node.  
            signedXml.LoadXml((XmlElement)nodeList[0]);

            return signedXml.CheckSignature(csp);
        }

        /// <summary>
        /// Verified the xml with the physical Certificate
        /// </summary>
        /// <param name="xmlDoc"> xml document</param>
        /// <param name="certificateFileName"> Certificate File Name</param>
        /// <returns>Boolean Value</returns>
        public static bool IsXmlSignValidByFile(XmlDocument xmlDoc, string certificateFileName)
        {
            CryptoConfig.AddAlgorithm(typeof(Security.Cryptography.RSAPKCS1SHA256SignatureDescription), "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
            CspParameters cspParams = new CspParameters();
            cspParams.KeyContainerName = "XML_DSIG_RSA_KEY";
            RSACryptoServiceProvider csp = new RSACryptoServiceProvider(cspParams);

            if (xmlDoc == null)
            {
                throw new CryptographicException("XML Document object cann't be null");
            }

            X509Certificate2 x509;

            // Add SHA-256 algorith mapping.
            CryptoConfig.AddAlgorithm(typeof(Security.Cryptography.RSAPKCS1SHA256SignatureDescription), XmlDsigSha256SignatureMethod);

            // Create a new x509 instance based on the passed in certificate.
            try
            {
                x509 = new X509Certificate2(certificateFileName, string.Empty, X509KeyStorageFlags.Exportable);
            }
            catch
            {
                throw new CryptographicException("Could not load the certificate file at " + certificateFileName);
            }

            // loading the key string
            csp.FromXmlString(x509.PublicKey.Key.ToXmlString(false));
            xmlDoc.PreserveWhitespace = true;

            SignedXml signedXml = new SignedXml(xmlDoc);
            signedXml.SignedInfo.CanonicalizationMethod = SignedXml.XmlDsigCanonicalizationUrl;
            signedXml.SignedInfo.SignatureMethod = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";
            signedXml.SigningKey = csp;

            // Create a reference to be signed.
            Reference reference = new Reference(string.Empty);
            reference.DigestMethod = "http://www.w3.org/2001/04/xmlenc#sha256";

            // Add an enveloped transformation to the reference.
            XmlDsigEnvelopedSignatureTransform env = new XmlDsigEnvelopedSignatureTransform(false);
            reference.AddTransform(env);
            signedXml.AddReference(reference);

            // Find the "Signature" node and create a new 
            XmlNodeList nodeList = xmlDoc.GetElementsByTagName("Signature");

            // Load the first <signature> node.  
            signedXml.LoadXml((XmlElement)nodeList[0]);

            return signedXml.CheckSignature(csp);
        }

        /// <summary>
        /// Signs XML document
        /// </summary>
        /// <param name="certificateFile"> Certificate file name with path</param>
        /// <param name="unsignedXmlDoc">xml document to be signed</param>
        /// <returns>xml document</returns>
        public static XmlDocument SignXmlDcoument(string certificateFile, XmlDocument unsignedXmlDoc)
        {
            if (unsignedXmlDoc == null)
            {
                throw new CryptographicException("XML Document object cann't be null");
            }

            X509Certificate2 x509;

            // Add SHA-256 algorith mapping.
            CryptoConfig.AddAlgorithm(typeof(Security.Cryptography.RSAPKCS1SHA256SignatureDescription), XmlDsigSha256SignatureMethod);

            // Create a new x509 instance based on the passed in certificate.
            try
            {
                x509 = new X509Certificate2(certificateFile, string.Empty, X509KeyStorageFlags.Exportable);
            }
            catch
            {
                throw new CryptographicException("Could not load the certificate file at " + certificateFile);
            }

            RSACryptoServiceProvider csp = null;

            // Get the private key of the certificate (used when signing)
            if (string.IsNullOrEmpty(cspName))
            {
                // The caller did not pass in a CSP name, so obtain the RSA CSP from x509 certificate
                CspParameters cspParams = new CspParameters(ProviderRsaAetType);
                cspParams.KeyContainerName = XmlDsigKeyContainerName;
                csp = new RSACryptoServiceProvider(cspParams);
                csp.FromXmlString(x509.PrivateKey.ToXmlString(true));
            }
            
            // Digitally Sign the XML document 
            SignXmlFile(unsignedXmlDoc, csp, x509);
            return unsignedXmlDoc;
        }

        /// <summary>
        /// Signs the specified XML document using the specified encryption.
        /// </summary>
        /// <param name="doc">The XML doc that is to be signed.</param>
        /// <param name="csp">The encryption method.</param>
        /// <param name="x509">x509 certificate</param>
        public static void SignXmlFile(XmlDocument doc, RSACryptoServiceProvider csp, X509Certificate2 x509)
        {
            // Generate a signing key.
            CryptoConfig.AddAlgorithm(typeof(Security.Cryptography.RSAPKCS1SHA256SignatureDescription), XmlDsigSha256SignatureMethod);

            SignedXml signedXml = new SignedXml(doc);
            signedXml.SignedInfo.CanonicalizationMethod = SignedXml.XmlDsigCanonicalizationUrl;
            signedXml.SignedInfo.SignatureMethod = XmlDsigSha256SignatureMethod;
            signedXml.SigningKey = csp;

            // Create a reference to be signed.
            Reference reference = new Reference(string.Empty);
            reference.DigestMethod = XmlSha256DigestMethod;

            // Add an enveloped transformation to the reference.
            XmlDsigEnvelopedSignatureTransform env = new XmlDsigEnvelopedSignatureTransform(false);
            reference.AddTransform(env);

            signedXml.AddReference(reference);

            // Add an RSAKeyValue KeyInfo (adds the public key to the key-value to help identify the enterprise).
            KeyInfo keyInfo = new KeyInfo();

            // Next add the Certificate
            keyInfo.AddClause(new KeyInfoX509Data(x509));
            signedXml.KeyInfo = keyInfo;

            // Compute the signature
            signedXml.ComputeSignature();

            // Get the XML representation of the signature and save
            // it to an XmlElement object.
            XmlElement xmlDigitalSignature = signedXml.GetXml();

            doc.DocumentElement.AppendChild(xmlDigitalSignature);
        }

        /// <summary>
        /// Loads an XML document. </summary>
        /// <param name="fileName">Unsigned XML being loaded.</param>
        /// <returns>Returns the unsigned XML document.</returns>
        private static XmlDocument LoadUnsignedXML(string fileName)
        {
            XmlDocument doc = new XmlDocument();

            doc.Load(fileName);

            return doc;
        }
    }
}

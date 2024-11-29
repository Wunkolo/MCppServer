#ifndef RSA_KEY_H
#define RSA_KEY_H

#include <memory>
#include <vector>
#include <openssl/evp.h>

class RSAKeyPair {
public:
    RSAKeyPair();
    ~RSAKeyPair();

    EVP_PKEY *getPublicKey() const;

    // Returns the public key in DER format
    std::vector<uint8_t> getPublicKeyDER() const;

    // Decrypts data using the private key
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encryptedData) const;

private:
    // Using EVP_PKEY for better abstraction
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey_;
};

#endif //RSA_KEY_H

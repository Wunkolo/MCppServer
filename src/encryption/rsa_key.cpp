#include "rsa_key.h"
#include <openssl/err.h>
#include <stdexcept>
#include <openssl/x509.h>

RSAKeyPair::RSAKeyPair()
    : pkey_(nullptr, EVP_PKEY_free)
{
    /// Generate RSA key using EVP_PKEY_Q_keygen
    // Parameters:
    // libctx = NULL (default library context)
    // propq = NULL (no properties)
    // type = "RSA"
    // key_size = 2048 (for 2048-bit RSA key)

    EVP_PKEY* generated_pkey = EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", static_cast<size_t>(2048));
    if (!generated_pkey) {
        // Retrieve and print OpenSSL error
        const unsigned long err = ERR_get_error();
        char errMsg[120];
        ERR_error_string_n(err, errMsg, sizeof(errMsg));
        throw std::runtime_error(std::string("EVP_PKEY_Q_keygen failed: ") + errMsg);
    }

    pkey_.reset(generated_pkey);
}

RSAKeyPair::~RSAKeyPair() {
    // EVP_PKEY_free is automatically called by unique_ptr
}

EVP_PKEY* RSAKeyPair::getPublicKey() const {
    return pkey_.get();
}

std::vector<uint8_t> RSAKeyPair::getPublicKeyDER() const {
    std::vector<uint8_t> der;

    // Create a memory BIO to hold the DER-encoded public key
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
        throw std::runtime_error("Failed to create BIO");
    }

    // Write the public key in DER format
    if (i2d_PUBKEY_bio(bio, pkey_.get()) != 1) {
        BIO_free(bio);
        throw std::runtime_error("Failed to write public key to BIO");
    }

    // Get the DER data from the BIO
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    if (!bufferPtr || !bufferPtr->data || bufferPtr->length == 0) {
        BIO_free(bio);
        throw std::runtime_error("Failed to get DER data from BIO");
    }

    der.assign(bufferPtr->data, bufferPtr->data + bufferPtr->length);
    BIO_free(bio);
    return der;
}

std::vector<uint8_t> RSAKeyPair::decrypt(const std::vector<uint8_t>& encryptedData) const {
    std::vector<uint8_t> decrypted(EVP_PKEY_size(pkey_.get()));
    std::size_t decryptedLen = decrypted.size();

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey_.get(), nullptr);
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_PKEY_CTX for decryption");
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption");
    }

    // Determine buffer length
    if (EVP_PKEY_decrypt(ctx, nullptr, &decryptedLen, encryptedData.data(), encryptedData.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to determine decrypted length");
    }

    decrypted.resize(decryptedLen);

    // Perform decryption
    if (EVP_PKEY_decrypt(ctx, decrypted.data(), &decryptedLen, encryptedData.data(), encryptedData.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Decryption failed");
    }

    decrypted.resize(decryptedLen);
    EVP_PKEY_CTX_free(ctx);
    return decrypted;
}
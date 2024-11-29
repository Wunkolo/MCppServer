// Original source:
// Copyright (c) 2020 Richard Hodges (hodges.r@gmail.com)
//
// Official repository: https://github.com/AlexAndDad/gateway
//
// Code was modified
#include "daft_hash.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <openssl/bn.h>

namespace minecraft::protocol
{
    daft_hash_impl::daft_hash_impl()
        : ctx_(EVP_MD_CTX_new(), EVP_MD_CTX_free)
    {
        if (!ctx_)
            throw std::runtime_error("EVP_MD_CTX_new failed");

        if (EVP_DigestInit_ex(ctx_.get(), EVP_sha1(), nullptr) != 1)
            throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    daft_hash_impl::~daft_hash_impl() = default;

    void daft_hash_impl::update(std::string_view in) const {
        if (EVP_DigestUpdate(ctx_.get(), in.data(), in.size()) != 1)
            throw std::runtime_error("EVP_DigestUpdate failed");
    }

    std::string daft_hash_impl::finalise() const {
        std::string result;

        // Finalize the digest
        unsigned char buf[EVP_MAX_MD_SIZE];
        unsigned int buf_len = 0;
        if (EVP_DigestFinal_ex(ctx_.get(), buf, &buf_len) != 1)
            throw std::runtime_error("EVP_DigestFinal_ex failed");

        // Convert hash to BIGNUM
        BIGNUM *bn = BN_bin2bn(buf, buf_len, nullptr);
        if (!bn)
            throw std::runtime_error("BN_bin2bn failed");

        // Reset the hasher for next use
        if (EVP_DigestInit_ex(ctx_.get(), EVP_sha1(), nullptr) != 1)
        {
            BN_free(bn);
            throw std::runtime_error("EVP_DigestInit_ex failed");
        }

        // Check for "negative" value (most significant bit)
        if (BN_is_bit_set(bn, buf_len * 8 - 1)) // Adjusted bit position
        {
            result += '-';

            // Perform 1's complement on the BIGNUM's bits
            int num_bytes = BN_num_bytes(bn);
            std::vector<unsigned char> tmp(num_bytes);
            if (BN_bn2bin(bn, tmp.data()) != num_bytes)
            {
                BN_free(bn);
                throw std::runtime_error("BN_bn2bin failed");
            }

            std::ranges::transform(tmp, tmp.begin(),
                                   [](unsigned char b) { return ~b; });

            // Free the original BIGNUM before reassigning
            BN_free(bn);
            bn = BN_bin2bn(tmp.data(), num_bytes, nullptr);
            if (!bn)
                throw std::runtime_error("BN_bin2bn (1's complement) failed");

            // Add 1 to perform 2's complement
            if (!BN_add_word(bn, 1))
            {
                BN_free(bn);
                throw std::runtime_error("BN_add_word failed");
            }
        }

        // Convert BIGNUM to hexadecimal string
        char *hex = BN_bn2hex(bn);
        if (!hex)
        {
            BN_free(bn);
            throw std::runtime_error("BN_bn2hex failed");
        }

        // Remove any leading zeroes except the last character
        std::string_view view(hex);
        while (view.size() > 1 && view[0] == '0')
            view.remove_prefix(1);

        // Append the hex to the result
        result.append(view.begin(), view.end());

        // Clean up
        OPENSSL_free(hex);
        BN_free(bn);

        // Convert the hex string to lower case using standard library
        std::ranges::transform(result, result.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return result;
    }

}   // namespace minecraft::protocol
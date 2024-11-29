// Original source:
// Copyright (c) 2020 Richard Hodges (hodges.r@gmail.com)
//
// Official repository: https://github.com/AlexAndDad/gateway
//
// Code was modified
#ifndef DAFT_HASH_H
#define DAFT_HASH_H
#include <openssl/evp.h>
#include <string>
#include <string_view>
#include <memory>

namespace minecraft::protocol
{
    struct daft_hash_impl
    {
        daft_hash_impl();

        daft_hash_impl(daft_hash_impl const &) = delete;
        daft_hash_impl(daft_hash_impl &&)      = delete;
        daft_hash_impl &operator=(daft_hash_impl const &) = delete;
        daft_hash_impl &operator=(daft_hash_impl &&) = delete;
        ~daft_hash_impl();

        void update(std::string_view in) const;

        std::string finalise() const;

    private:
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx_;
    };
}   // namespace minecraft::protocol
#endif //DAFT_HASH_H
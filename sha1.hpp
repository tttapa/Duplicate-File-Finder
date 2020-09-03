#pragma once

#include <filesystem>
#include <fstream>
#include <openssl/evp.h>
#include <vector>

/// RAII wrapper for OpenSSL's SHA-1 hasher. Can hash any data or files.
class SHA1_Digester {
  public:
    using hash_t = std::vector<unsigned char>;

    SHA1_Digester() { EVP_DigestInit_ex(mdctx, md, NULL); }
    ~SHA1_Digester() { EVP_MD_CTX_free(mdctx); }

    /// Reset and re-initialize the digester context.
    void reset() {
        EVP_MD_CTX_reset(mdctx);
        EVP_DigestInit_ex(mdctx, md, NULL);
    }

    /// Digest some raw data.
    void digest(const void *data, std::size_t length) {
        EVP_DigestUpdate(mdctx, data, length);
    }

    /// Finalize the digestion and return the hash.
    hash_t get() const {
        hash_t md_value(EVP_MD_size(md));
        EVP_DigestFinal(mdctx, md_value.data(), nullptr);
        return md_value;
    }

    /// Digest a file and return the hash.
    hash_t digest_file(const std::filesystem::path &path) {
        std::ifstream f(path);
        buff.resize(1024ul * EVP_MD_CTX_block_size(mdctx));
        while (f) {
            auto sz = f.read(buff.data(), buff.size()).gcount();
            digest(buff.data(), sz);
        }
        return get();
    }

  private:
    const EVP_MD *md = EVP_sha1();
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    std::vector<char> buff;
};
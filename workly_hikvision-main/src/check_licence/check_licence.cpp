#include "check_licence.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <system_error>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/hmac.h>

unsigned char key[] = {
	0x18, 0x92, 0x35, 0x99, 0x59, 0x7B, 0x0D, 0x07,
	0x4F, 0x6C, 0x8E, 0x4A, 0x85, 0x7B, 0x08, 0xE9,
	0xB7, 0x48, 0x35, 0x0F, 0x60, 0x08, 0x1F, 0xD0,
	0x42, 0x03, 0xBF, 0xEF, 0x1B, 0x3A, 0x80, 0x39,
	0x00, 0xEF, 0xB3, 0x58, 0x4F, 0x15, 0xDB, 0xFD,
	0xF6, 0xFC, 0x9A, 0xF6, 0x0E, 0x2F, 0xA5, 0xED,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void hmac_sha256(const unsigned char* text, int text_len, const unsigned char* key, int key_len, void* digest);

namespace licence {
    
    void validate(std::string filename) {

        std::ifstream file(filename, std::ios_base::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open token file: " + filename);
        }
        
        unsigned char token[40];
        file.read((char*)token, 40);
        if (file.gcount() != 40) {
            throw std::runtime_error("invalid token size");
        }

        unsigned long long ull_expires_at;
        std::memcpy(&ull_expires_at, token, 8);

        std::time_t now = std::time(nullptr);
        if (now >= ull_expires_at) {
            throw std::runtime_error("token expired");
        }

        std::ifstream file1("/sys/class/dmi/id/product_uuid");
        std::ifstream file2("/sys/class/dmi/id/product_serial");
        if (!file1.is_open() || !file2.is_open()) {
            throw std::runtime_error("permission denied. please run as an administrator");
        }
        
        std::string product_uuid, product_serial;
        file1 >> product_uuid;
        file2 >> product_serial;
        
        std::string uuid = product_uuid + product_serial;
        std::memcpy(key + 48, &ull_expires_at, 8);
        unsigned char sign[32];
        std::memset(sign, 0, sizeof(32));
        hmac_sha256((const unsigned char*)uuid.data(), uuid.size(), key, sizeof(key), sign);

        if (std::memcmp(token + 8, sign, 32) == 0) {
            std::time_t expires_at = (std::time_t)ull_expires_at;
            struct tm* t = localtime(&expires_at);
            printf("Token valid. Expires at %04d-%02d-%02d %02d:%02d:%02d\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);
        }
        else {
            throw std::runtime_error("verify failed");
        }
    }

    std::string generate_temp_token() {
        
        std::ifstream file1("/sys/class/dmi/id/product_uuid");
        std::ifstream file2("/sys/class/dmi/id/product_serial");
        if (!file1.is_open() || !file2.is_open()) {
            throw std::runtime_error("permission denied. please run as an administrator");
        }
        
        std::string product_uuid, product_serial;
        file1 >> product_uuid;
        file2 >> product_serial;
        
        std::string uuid = product_uuid + product_serial;

        int key_len = sizeof(key);
        std::string temp_token;
        for (int i = 0; i < uuid.size(); ++i) {
            unsigned char ch = (unsigned char)uuid.data()[i];
            ch = ch ^ key[i % key_len];
            char buf[3];
            sprintf(buf, "%02x", ch);
            temp_token += buf;
        }

        return temp_token;
    }

}

void
hmac_sha256(
const unsigned char *text,      /* pointer to data stream        */
    int                 text_len,   /* length of data stream         */
    const unsigned char *key,       /* pointer to authentication key */
    int                 key_len,    /* length of authentication key  */
    void                *digest)    /* caller digest to be filled in */
{
    unsigned char k_ipad[65];   /* inner padding -
                                 * key XORd with ipad
                                 */
    unsigned char k_opad[65];   /* outer padding -
                                 * key XORd with opad
                                 */
    unsigned char tk[SHA256_DIGEST_LENGTH];
    unsigned char tk2[SHA256_DIGEST_LENGTH];
    unsigned char bufferIn[1024];
    unsigned char bufferOut[1024];
    int           i;

    /* if key is longer than 64 bytes reset it to key=sha256(key) */
    if ( key_len > 64 ) {
        SHA256( key, key_len, tk );
        key     = tk;
        key_len = SHA256_DIGEST_LENGTH;
    }

    /*
     * the HMAC_SHA256 transform looks like:
     *
     * SHA256(K XOR opad, SHA256(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    /* start out by storing key in pads */
    memset( k_ipad, 0, sizeof k_ipad );
    memset( k_opad, 0, sizeof k_opad );
    memcpy( k_ipad, key, key_len );
    memcpy( k_opad, key, key_len );

    /* XOR key with ipad and opad values */
    for ( i = 0; i < 64; i++ ) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /*
     * perform inner SHA256
     */
    memset( bufferIn, 0x00, 1024 );
    memcpy( bufferIn, k_ipad, 64 );
    memcpy( bufferIn + 64, text, text_len );

    SHA256( bufferIn, 64 + text_len, tk2 );

    /*
     * perform outer SHA256
     */
    memset( bufferOut, 0x00, 1024 );
    memcpy( bufferOut, k_opad, 64 );
    memcpy( bufferOut + 64, tk2, SHA256_DIGEST_LENGTH );

    SHA256( bufferOut, 64 + SHA256_DIGEST_LENGTH, (unsigned char*)digest );
}


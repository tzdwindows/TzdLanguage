#include "TzdInterpreter.h"
#include "TzdCryptoModule.h"

#include <windows.h>
#include <wincrypt.h>
#include <rpc.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "rpcrt4.lib")

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <mutex>

static HCRYPTPROV g_hCryptProv = NULL;
static std::once_flag g_cryptInitOnce;

static void ensureCryptProv() {
    std::call_once(g_cryptInitOnce, []() {
        if (!CryptAcquireContextW(&g_hCryptProv, NULL, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            CryptAcquireContextW(&g_hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
        }
    });
}

static std::string computeHash(ALG_ID alg, const std::string& input) {
    ensureCryptProv();
    if (!g_hCryptProv) return "";

    HCRYPTHASH hHash = NULL;
    if (!CryptCreateHash(g_hCryptProv, alg, 0, 0, &hHash)) return "";

    if (!CryptHashData(hHash, (const BYTE*)input.data(), (DWORD)input.size(), 0)) {
        CryptDestroyHash(hHash);
        return "";
    }

    DWORD hashLen = 0;
    DWORD sizeLen = sizeof(hashLen);
    CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &sizeLen, 0);

    std::vector<BYTE> hashData(hashLen);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &hashLen, 0)) {
        CryptDestroyHash(hHash);
        return "";
    }
    CryptDestroyHash(hHash);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (BYTE b : hashData) {
        oss << std::setw(2) << (int)b;
    }
    return oss.str();
}

static std::string base64Encode(const std::string& input) {
    if (input.empty()) return "";
    DWORD len = 0;
    CryptBinaryToStringA((const BYTE*)input.data(), (DWORD)input.size(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &len);
    std::string out(len, 0);
    CryptBinaryToStringA((const BYTE*)input.data(), (DWORD)input.size(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0], &len);
    while (!out.empty() && (out.back() == '\0' || out.back() == '\r' || out.back() == '\n')) {
        out.pop_back();
    }
    return out;
}

static std::string base64Decode(const std::string& input) {
    if (input.empty()) return "";
    DWORD len = 0;
    CryptStringToBinaryA(input.data(), (DWORD)input.size(), CRYPT_STRING_BASE64, NULL, &len, NULL, NULL);
    std::string out(len, 0);
    CryptStringToBinaryA(input.data(), (DWORD)input.size(), CRYPT_STRING_BASE64, (BYTE*)&out[0], &len, NULL, NULL);
    return out;
}

static std::string hexEncode(const std::string& input) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        oss << std::setw(2) << (int)c;
    }
    return oss.str();
}

static std::string hexDecode(const std::string& input) {
    std::string out;
    out.reserve(input.size() / 2);
    for (size_t i = 0; i + 1 < input.size(); i += 2) {
        std::string byteStr = input.substr(i, 2);
        char byteVal = (char)std::strtol(byteStr.c_str(), NULL, 16);
        out.push_back(byteVal);
    }
    return out;
}

void TzdCryptoModule::init(TzdInterpreter* interp) {
    ensureCryptProv();

    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f);
        v.name = name;
        interp->setGlobalVariable(name, v);
    };

    reg("crypto_md5", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(computeHash(CALG_MD5, TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_sha1", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(computeHash(CALG_SHA1, TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_sha256", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(computeHash(CALG_SHA_256, TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_sha512", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(computeHash(CALG_SHA_512, TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_base64_encode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(base64Encode(TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_base64_decode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(base64Decode(TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_hex_encode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(hexEncode(TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_hex_decode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(hexDecode(TzdInterpreter::getAsString(args[0])));
    });

    reg("crypto_uuid", [](const std::vector<TzdValue>&) -> TzdValue {
        UUID uuid;
        UuidCreate(&uuid);
        RPC_CSTR rpcStr = NULL;
        UuidToStringA(&uuid, &rpcStr);
        std::string res;
        if (rpcStr) {
            res = (char*)rpcStr;
            RpcStringFreeA(&rpcStr);
        }
        return TzdValue(res);
    });

    reg("crypto_random_bytes", [](const std::vector<TzdValue>& args) -> TzdValue {
        int len = (!args.empty()) ? (int)TzdInterpreter::getAsDoubleInternal(args[0]) : 16;
        if (len <= 0) len = 16;
        ensureCryptProv();
        std::vector<BYTE> buf(len);
        if (g_hCryptProv && CryptGenRandom(g_hCryptProv, len, buf.data())) {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (BYTE b : buf) oss << std::setw(2) << (int)b;
            return TzdValue(oss.str());
        }
        return TzdValue("");
    });
}

#define _WINSOCKAPI_

#include "TzdInterpreter.h"
#include "TzdNetModule.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <mutex>
#include <atomic>

static std::once_flag g_wsaInitOnce;
static void ensureWsaInit() {
    std::call_once(g_wsaInitOnce, []() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    });
}

// Convert UTF-8 std::string to std::wstring
static std::wstring utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

// Convert std::wstring to UTF-8 std::string
static std::string wideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], sizeNeeded, NULL, NULL);
    return str;
}

// Socket handle wrapper
static std::unordered_map<long long, SOCKET> g_activeSockets;
static std::mutex g_socketMutex;
static std::atomic<long long> g_socketIdGen{1};

static long long registerSocket(SOCKET s) {
    std::lock_guard<std::mutex> lock(g_socketMutex);
    long long id = g_socketIdGen.fetch_add(1);
    g_activeSockets[id] = s;
    return id;
}

static SOCKET getSocket(long long id) {
    std::lock_guard<std::mutex> lock(g_socketMutex);
    auto it = g_activeSockets.find(id);
    if (it != g_activeSockets.end()) return it->second;
    return INVALID_SOCKET;
}

static void unregisterSocket(long long id) {
    std::lock_guard<std::mutex> lock(g_socketMutex);
    auto it = g_activeSockets.find(id);
    if (it != g_activeSockets.end()) {
        closesocket(it->second);
        g_activeSockets.erase(it);
    }
}

// URL encode / decode
static std::string urlEncode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (unsigned char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << std::uppercase << (int)c;
        }
    }
    return escaped.str();
}

static std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int val = 0;
            std::istringstream is(str.substr(i + 1, 2));
            if (is >> std::hex >> val) {
                result += (char)val;
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

// Full HTTP Request via WinHttp
static TzdValue doHttpRequest(const std::string& method, const std::string& url,
                              const std::unordered_map<std::string, TzdValue>& headers,
                              const std::string& body, int timeoutMs) {
    std::unordered_map<std::string, TzdValue> resMap;
    resMap["status"] = TzdValue((double)0);
    resMap["statusText"] = TzdValue("");
    resMap["body"] = TzdValue("");
    resMap["ok"] = TzdValue(false);
    resMap["error"] = TzdValue("");
    std::unordered_map<std::string, TzdValue> respHeaders;
    resMap["headers"] = TzdValue(respHeaders);

    std::wstring wUrl = utf8ToWide(url);
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[512] = {0};
    wchar_t urlPath[4096] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 512;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 4096;

    if (!WinHttpCrackUrl(wUrl.c_str(), (DWORD)wUrl.length(), 0, &urlComp)) {
        resMap["error"] = TzdValue("Invalid URL: failed to crack URL components.");
        return TzdValue(resMap);
    }

    HINTERNET hSession = WinHttpOpen(L"TzdTools-NetClient/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        resMap["error"] = TzdValue("Failed to initialize WinHttp session.");
        return TzdValue(resMap);
    }

    int t = (timeoutMs > 0) ? timeoutMs : 30000;
    WinHttpSetTimeouts(hSession, t, t, t, t);

    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        resMap["error"] = TzdValue("Failed to connect to host.");
        return TzdValue(resMap);
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    std::wstring wMethod = utf8ToWide(method);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(), urlComp.lpszUrlPath,
                                           NULL, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        resMap["error"] = TzdValue("Failed to open HTTP request.");
        return TzdValue(resMap);
    }

    // Build extra headers
    std::wstring extraHeadersW;
    for (const auto& kv : headers) {
        std::string line = kv.first + ": " + TzdInterpreter::getAsString(kv.second) + "\r\n";
        extraHeadersW += utf8ToWide(line);
    }

    // Auto set Content-Type if not set and body is present
    if (!body.empty() && extraHeadersW.find(L"Content-Type:") == std::wstring::npos) {
        extraHeadersW += L"Content-Type: application/json; charset=utf-8\r\n";
    }

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       extraHeadersW.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : extraHeadersW.c_str(),
                                       (DWORD)extraHeadersW.length(),
                                       body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data(),
                                       (DWORD)body.size(),
                                       (DWORD)body.size(), 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    if (!bResults) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        resMap["error"] = TzdValue("HTTP Request failed. Error code: " + std::to_string(err));
        return TzdValue(resMap);
    }

    // Query status code
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
    resMap["status"] = TzdValue((double)statusCode);
    resMap["ok"] = TzdValue(statusCode >= 200 && statusCode < 300);

    // Query status text
    wchar_t statusTextBuf[256] = {0};
    DWORD statusTextSize = sizeof(statusTextBuf);
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_TEXT,
                            WINHTTP_HEADER_NAME_BY_INDEX, statusTextBuf, &statusTextSize, WINHTTP_NO_HEADER_INDEX)) {
        resMap["statusText"] = TzdValue(wideToUtf8(statusTextBuf));
    }

    // Query all response headers
    DWORD headerSize = 0;
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &headerSize, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && headerSize > 0) {
        std::vector<wchar_t> headerBuf(headerSize / sizeof(wchar_t) + 1, 0);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                WINHTTP_HEADER_NAME_BY_INDEX, headerBuf.data(), &headerSize, WINHTTP_NO_HEADER_INDEX)) {
            std::string allHeaders = wideToUtf8(headerBuf.data());
            std::istringstream stream(allHeaders);
            std::string hLine;
            while (std::getline(stream, hLine)) {
                if (!hLine.empty() && hLine.back() == '\r') hLine.pop_back();
                size_t colon = hLine.find(':');
                if (colon != std::string::npos) {
                    std::string key = hLine.substr(0, colon);
                    std::string val = hLine.substr(colon + 1);
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    val.erase(0, val.find_first_not_of(" \t"));
                    val.erase(val.find_last_not_of(" \t") + 1);
                    respHeaders[key] = TzdValue(val);
                }
            }
            resMap["headers"] = TzdValue(respHeaders);
        }
    }

    // Read response body
    std::string responseBody;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead) && bytesRead > 0) {
            responseBody.append(buffer.data(), bytesRead);
        } else {
            break;
        }
    }
    resMap["body"] = TzdValue(responseBody);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return TzdValue(resMap);
}

void TzdNetModule::init(TzdInterpreter* interp) {
    ensureWsaInit();

    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f);
        v.name = name;
        interp->setGlobalVariable(name, v);
    };

    // -------------------------------------------------------------
    // HTTP API
    // -------------------------------------------------------------
    reg("net_http_request", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("net_http_request(method, url, [headers], [body], [timeoutMs]) requires at least method and url.");
        std::string method = TzdInterpreter::getAsString(args[0]);
        std::string url = TzdInterpreter::getAsString(args[1]);
        std::unordered_map<std::string, TzdValue> headers;
        if (args.size() > 2 && args[2].type == TzdValue::MAP) {
            headers = args[2].mapVal;
        }
        std::string body = (args.size() > 3) ? TzdInterpreter::getAsString(args[3]) : "";
        int timeoutMs = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 30000;
        return doHttpRequest(method, url, headers, body, timeoutMs);
    });

    reg("net_http_get", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("net_http_get(url, [headers], [timeoutMs]) requires url.");
        std::string url = TzdInterpreter::getAsString(args[0]);
        std::unordered_map<std::string, TzdValue> headers;
        if (args.size() > 1 && args[1].type == TzdValue::MAP) headers = args[1].mapVal;
        int timeoutMs = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 30000;
        return doHttpRequest("GET", url, headers, "", timeoutMs);
    });

    reg("net_http_post", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("net_http_post(url, body, [headers], [timeoutMs]) requires url and body.");
        std::string url = TzdInterpreter::getAsString(args[0]);
        std::string body = TzdInterpreter::getAsString(args[1]);
        std::unordered_map<std::string, TzdValue> headers;
        if (args.size() > 2 && args[2].type == TzdValue::MAP) headers = args[2].mapVal;
        int timeoutMs = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 30000;
        return doHttpRequest("POST", url, headers, body, timeoutMs);
    });

    reg("net_http_put", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("net_http_put(url, body, [headers], [timeoutMs]) requires url and body.");
        std::string url = TzdInterpreter::getAsString(args[0]);
        std::string body = TzdInterpreter::getAsString(args[1]);
        std::unordered_map<std::string, TzdValue> headers;
        if (args.size() > 2 && args[2].type == TzdValue::MAP) headers = args[2].mapVal;
        int timeoutMs = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 30000;
        return doHttpRequest("PUT", url, headers, body, timeoutMs);
    });

    reg("net_http_delete", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("net_http_delete(url, [headers], [timeoutMs]) requires url.");
        std::string url = TzdInterpreter::getAsString(args[0]);
        std::unordered_map<std::string, TzdValue> headers;
        if (args.size() > 1 && args[1].type == TzdValue::MAP) headers = args[1].mapVal;
        int timeoutMs = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 30000;
        return doHttpRequest("DELETE", url, headers, "", timeoutMs);
    });

    reg("net_http_download", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("net_http_download(url, destPath, [timeoutMs]) requires url and destPath.");
        std::string url = TzdInterpreter::getAsString(args[0]);
        std::string destPath = TzdInterpreter::getAsString(args[1]);
        int timeoutMs = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 60000;

        TzdValue resp = doHttpRequest("GET", url, {}, "", timeoutMs);
        if (resp.type == TzdValue::MAP && resp.mapVal["ok"].bVal) {
            std::ofstream out(destPath, std::ios::binary);
            if (!out.is_open()) return TzdValue(false);
            const std::string& b = resp.mapVal["body"].sVal;
            out.write(b.data(), b.size());
            out.close();
            return TzdValue(true);
        }
        return TzdValue(false);
    });

    // -------------------------------------------------------------
    // TCP Socket API
    // -------------------------------------------------------------
    reg("net_tcp_connect", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("net_tcp_connect(host, port, [timeoutMs]) requires host and port.");
        std::string host = TzdInterpreter::getAsString(args[0]);
        int port = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int timeoutMs = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10000;

        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return TzdValue((double)-1);

        // Resolve host
        struct addrinfo hints, *res = nullptr;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0 || !res) {
            closesocket(s);
            return TzdValue((double)-1);
        }

        // Set non-blocking for timeout connect
        u_long mode = 1;
        ioctlsocket(s, FIONBIO, &mode);

        connect(s, res->ai_addr, (int)res->ai_addrlen);
        freeaddrinfo(res);

        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(s, &writeSet);
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int selRes = select(0, NULL, &writeSet, NULL, &tv);
        if (selRes <= 0) {
            closesocket(s);
            return TzdValue((double)-1);
        }

        // Check if actually connected
        int optVal = 0;
        int optLen = sizeof(optVal);
        getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&optVal, &optLen);
        if (optVal != 0) {
            closesocket(s);
            return TzdValue((double)-1);
        }

        // Set back to blocking mode
        mode = 0;
        ioctlsocket(s, FIONBIO, &mode);

        long long id = registerSocket(s);
        return TzdValue((double)id);
    });

    reg("net_tcp_send", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("net_tcp_send(handle, data) requires handle and data.");
        long long id = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        SOCKET s = getSocket(id);
        if (s == INVALID_SOCKET) return TzdValue((double)-1);

        std::string data = TzdInterpreter::getAsString(args[1]);
        int sent = send(s, data.data(), (int)data.size(), 0);
        return TzdValue((double)sent);
    });

    reg("net_tcp_recv", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("net_tcp_recv(handle, [maxBytes], [timeoutMs]) requires handle.");
        long long id = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        SOCKET s = getSocket(id);
        if (s == INVALID_SOCKET) return TzdValue("");

        int maxBytes = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 4096;
        if (maxBytes <= 0) maxBytes = 4096;
        int timeoutMs = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 5000;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(s, &readSet);
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int sel = select(0, &readSet, NULL, NULL, &tv);
        if (sel <= 0) return TzdValue("");

        std::vector<char> buf(maxBytes);
        int recvd = recv(s, buf.data(), maxBytes, 0);
        if (recvd <= 0) return TzdValue("");
        return TzdValue(std::string(buf.data(), recvd));
    });

    reg("net_tcp_close", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        long long id = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        unregisterSocket(id);
        return TzdValue(true);
    });

    reg("net_tcp_listen", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("net_tcp_listen(port, [host], [backlog]) requires port.");
        int port = (int)TzdInterpreter::getAsDoubleInternal(args[0]);
        std::string host = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "0.0.0.0";
        int backlog = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 128;

        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return TzdValue((double)-1);

        int opt = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((u_short)port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
            listen(s, backlog) == SOCKET_ERROR) {
            closesocket(s);
            return TzdValue((double)-1);
        }

        long long id = registerSocket(s);
        return TzdValue((double)id);
    });

    reg("net_tcp_accept", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("net_tcp_accept(serverHandle, [timeoutMs]) requires serverHandle.");
        long long srvId = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        SOCKET s = getSocket(srvId);
        if (s == INVALID_SOCKET) return TzdValue();

        int timeoutMs = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 5000;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(s, &readSet);
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int sel = select(0, &readSet, NULL, NULL, &tv);
        if (sel <= 0) return TzdValue();

        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(s, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock == INVALID_SOCKET) return TzdValue();

        char ipBuf[64] = {0};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
        int clientPort = ntohs(clientAddr.sin_port);

        long long clientId = registerSocket(clientSock);
        std::unordered_map<std::string, TzdValue> res;
        res["client"] = TzdValue((double)clientId);
        res["ip"] = TzdValue(std::string(ipBuf));
        res["port"] = TzdValue((double)clientPort);
        return TzdValue(res);
    });

    // -------------------------------------------------------------
    // UDP Socket API
    // -------------------------------------------------------------
    reg("net_udp_bind", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("net_udp_bind(port, [host]) requires port.");
        int port = (int)TzdInterpreter::getAsDoubleInternal(args[0]);
        std::string host = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "0.0.0.0";

        SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == INVALID_SOCKET) return TzdValue((double)-1);

        sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((u_short)port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(s);
            return TzdValue((double)-1);
        }

        long long id = registerSocket(s);
        return TzdValue((double)id);
    });

    reg("net_udp_sendto", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 4) return TzdValue::Error("net_udp_sendto(handle, data, host, port) requires 4 args.");
        long long id = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        SOCKET s = getSocket(id);
        if (s == INVALID_SOCKET) {
            s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (s == INVALID_SOCKET) return TzdValue((double)-1);
        }

        std::string data = TzdInterpreter::getAsString(args[1]);
        std::string host = TzdInterpreter::getAsString(args[2]);
        int port = (int)TzdInterpreter::getAsDoubleInternal(args[3]);

        sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((u_short)port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        int sent = sendto(s, data.data(), (int)data.size(), 0, (sockaddr*)&addr, sizeof(addr));
        return TzdValue((double)sent);
    });

    reg("net_udp_recvfrom", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("net_udp_recvfrom(handle, [maxBytes], [timeoutMs]) requires handle.");
        long long id = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        SOCKET s = getSocket(id);
        if (s == INVALID_SOCKET) return TzdValue();

        int maxBytes = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 4096;
        int timeoutMs = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 5000;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(s, &readSet);
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int sel = select(0, &readSet, NULL, NULL, &tv);
        if (sel <= 0) return TzdValue();

        std::vector<char> buf(maxBytes);
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        int recvd = recvfrom(s, buf.data(), maxBytes, 0, (sockaddr*)&fromAddr, &fromLen);
        if (recvd <= 0) return TzdValue();

        char ipBuf[64] = {0};
        inet_ntop(AF_INET, &fromAddr.sin_addr, ipBuf, sizeof(ipBuf));

        std::unordered_map<std::string, TzdValue> res;
        res["data"] = TzdValue(std::string(buf.data(), recvd));
        res["ip"] = TzdValue(std::string(ipBuf));
        res["port"] = TzdValue((double)ntohs(fromAddr.sin_port));
        return TzdValue(res);
    });

    reg("net_udp_close", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        long long id = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        unregisterSocket(id);
        return TzdValue(true);
    });

    // -------------------------------------------------------------
    // URL Utilities & System Network Info
    // -------------------------------------------------------------
    reg("net_url_encode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(urlEncode(TzdInterpreter::getAsString(args[0])));
    });

    reg("net_url_decode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        return TzdValue(urlDecode(TzdInterpreter::getAsString(args[0])));
    });

    reg("net_url_parse", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue();
        std::string url = TzdInterpreter::getAsString(args[0]);
        std::wstring wUrl = utf8ToWide(url);
        URL_COMPONENTS uc;
        ZeroMemory(&uc, sizeof(uc));
        uc.dwStructSize = sizeof(uc);
        wchar_t scheme[32] = {0};
        wchar_t host[256] = {0};
        wchar_t path[2048] = {0};
        wchar_t extra[2048] = {0};
        uc.lpszScheme = scheme; uc.dwSchemeLength = 32;
        uc.lpszHostName = host; uc.dwHostNameLength = 256;
        uc.lpszUrlPath = path;  uc.dwUrlPathLength = 2048;
        uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 2048;

        std::unordered_map<std::string, TzdValue> res;
        if (WinHttpCrackUrl(wUrl.c_str(), (DWORD)wUrl.length(), 0, &uc)) {
            res["protocol"] = TzdValue(wideToUtf8(scheme));
            res["host"] = TzdValue(wideToUtf8(host));
            res["port"] = TzdValue((double)uc.nPort);
            res["path"] = TzdValue(wideToUtf8(path));
            std::string extraStr = wideToUtf8(extra);
            size_t hashPos = extraStr.find('#');
            if (hashPos != std::string::npos) {
                res["query"] = TzdValue(extraStr.substr(0, hashPos));
                res["fragment"] = TzdValue(extraStr.substr(hashPos + 1));
            } else {
                res["query"] = TzdValue(extraStr);
                res["fragment"] = TzdValue("");
            }
        }
        return TzdValue(res);
    });

    reg("net_get_hostname", [](const std::vector<TzdValue>&) -> TzdValue {
        char name[256] = {0};
        gethostname(name, sizeof(name));
        return TzdValue(std::string(name));
    });

    reg("net_resolve_ip", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string host = TzdInterpreter::getAsString(args[0]);
        struct addrinfo hints, *res = nullptr;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        if (getaddrinfo(host.c_str(), NULL, &hints, &res) == 0 && res) {
            sockaddr_in* addr = (sockaddr_in*)res->ai_addr;
            char ipBuf[64] = {0};
            inet_ntop(AF_INET, &addr->sin_addr, ipBuf, sizeof(ipBuf));
            freeaddrinfo(res);
            return TzdValue(std::string(ipBuf));
        }
        return TzdValue("");
    });
}

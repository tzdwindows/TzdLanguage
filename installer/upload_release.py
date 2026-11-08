# -*- coding: utf-8 -*-
"""
upload_release.py
Automated GitHub Release creation and Asset Upload for TzdTools v0.2.9
"""

import os
import sys
import json
import subprocess
import urllib.request
import urllib.parse
import urllib.error

TAG = "v0.2.14"
RELEASE_NAME = "TzdTools v0.2.14 - 极致动态内存压缩、全套原生系统模块 (Hardware/Benchmark/Encoding/ByteBuffer) 与双架构发行版"
REPO_OWNER = "tzdwindows"
REPO_NAME = "TzdLanguage"
PROXY_CANDIDATES = [
    "http://127.0.0.1:10808",
    "http://127.0.0.1:7897",
    "http://127.0.0.1:7890",
]

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Check proxy availability
def find_available_proxy():
    import socket
    for p in PROXY_CANDIDATES:
        try:
            parsed = urllib.parse.urlparse(p)
            with socket.create_connection((parsed.hostname, parsed.port), timeout=1.0):
                return p
        except Exception:
            continue
    return None

detected_proxy = find_available_proxy()
if detected_proxy:
    print(f"  -> Using proxy {detected_proxy}")
    proxy_handler = urllib.request.ProxyHandler({'http': detected_proxy, 'https': detected_proxy})
    opener = urllib.request.build_opener(proxy_handler)
    urllib.request.install_opener(opener)
else:
    print("  -> Direct connection (proxy not detected)")

# 1. Get Token from Git Credential Manager
print("[1/4] Retrieving GitHub credentials from git-credential-manager...")
proc = subprocess.run(
    ["git", "credential-manager", "get"],
    input=b"protocol=https\nhost=github.com\n\n",
    capture_output=True,
    check=True
)
lines = proc.stdout.decode("utf-8").splitlines()
token = None
for line in lines:
    if line.startswith("password="):
        token = line[len("password="):].strip()
        break

if not token:
    print("Error: Could not retrieve token from git-credential-manager", file=sys.stderr)
    sys.exit(1)
print(f"  -> Token retrieved successfully for {REPO_OWNER}")

headers = {
    "Authorization": f"Bearer {token}",
    "Accept": "application/vnd.github+json",
    "User-Agent": "TzdTools-Release-Uploader"
}

# 2. Find or Create Release
print(f"[2/4] Checking or creating GitHub Release for {TAG}...")
get_url = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases/tags/{TAG}"
release = None

try:
    req = urllib.request.Request(get_url, headers=headers)
    with urllib.request.urlopen(req) as resp:
        release = json.loads(resp.read().decode("utf-8"))
        print(f"  -> Found existing release ID: {release['id']}")
except urllib.error.HTTPError as e:
    if e.code == 404:
        print(f"  -> Release {TAG} does not exist yet. Creating...")
        create_url = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases"
        body_text = """## 🚀 TzdTools v0.2.14 发布说明

### 💎 核心亮点

1. **TzdValue 内存对象极致动态压缩 (Dynamic Payload Compression, DPC)**：
   - 彻底优化值对象结构，基础内存占用从 608 字节极致压缩至仅 48 字节（**缩减超 92%**），突破理论内存布局极限
   - 引入动态压缩引擎：空闲字段紧凑化、标量/布尔就地存储、实例动态解包与按需写时解压（Copy-On-Write Decompression）
   - 引入动态压缩生命周期事件与对象观测探针机制（`tzd_ti_set_compression_event_callback`），全透明监控压缩前后对象与堆内存变化

2. **全套工业级原生系统模块 (Native Built-in Modules)**：
   - **`Hardware` 模块**：CPU 品牌型号识别、物理/逻辑核心数检测、AVX2/AVX512 指令集支持检测、物理内存总量/进程物理内存与虚拟内存实时监控、GPU 设备数量与显存监控
   - **`Benchmark` 模块**：纳秒级高精度计时（`Benchmark.nanoTime()`）、微秒计时、自动化代码块耗时基准评测（`Benchmark.measure`）
   - **`Encoding` 模块**：高效 Base64 编解码、Hex 十六进制编解码、CRC-32 与 Adler-32 校验和快速计算
   - **`ByteBuffer` 模块**：高性能直接堆外字节缓冲区，提供对 I8/I16/I32/I64/Float/Double 及 UTF-8 字符串的原生读写，支持游标定位、切片与容量管理

3. **类 JNI 原生接口 TNI 与官方 Java 绑定全面升级**：
   - TNI 导出直接字节缓冲区操作（`NewDirectByteBuffer`, `GetDirectBufferAddress`, `GetDirectBufferCapacity`）与全局/局部引用管理
   - 官方 Java 绑定 `tzd-java-sdk.jar` 全面升级，支持从 Java 访问 `Hardware` 硬件信息、`Benchmark` 高精度纳秒计时器、类热重载（`retransformClass`）及直接字节流互操作

4. **双架构发行版（GPU 完整版 + CPU 纯享版）**：
   - **GPU 旗舰版** (`TzdTools_Setup_v0.2.14.exe`)：内置全套 CUDA 12.6 与 LibTorch 深度学习运行时，开箱即用支持多卡 GPU 神经网络训练与推理、NTT 大数运算
   - **CPU 纯享版** (`TzdTools_Setup_v0.2.14_CPU.exe`)：极致轻量（~80 MB 安装包），免去 Gigabytes 级别的 CUDA 依赖，极速下载安装，专为 CPU 运算与标准脚本开发打造

5. **标准化轻量 SDK 与 VS Code 生态**：
   - 便携 SDK 压缩包 `TzdSDK-v0.2.14-windows-x64.zip`：包含 C/C++ 核心头文件、导入库 `tzd.lib`、动态库 `tzd.dll`、Java SDK `tzd-java-sdk.jar` 及许可证
   - 官方 VS Code 扩展 `tzdlang-0.2.21.vsix`：语法高亮、LSP 代码补全与 DAP 调试器支持

---

### 📦 资产列表 (Release Assets)
- **TzdTools_Setup_v0.2.14.exe**：包含全套 CUDA 12.6 运行库、LibTorch GPU 运行时、编译器的 Windows 完整安装包 (~1.4 GB)
- **TzdTools_Setup_v0.2.14_CPU.exe**：轻量级 CPU 纯享版 Windows 安装包 (~80 MB)
- **TzdSDK-v0.2.14-windows-x64.zip**：面向 C/C++ 及 Java 开发者的高性能轻量 SDK (头文件、`tzd.lib`、`tzd.dll`、`tzd-java-sdk.jar`)
- **tzdlang-0.2.21.vsix**：VS Code 官方语言扩展（语法高亮、LSP 智能感知、DAP 调试器）
"""
        payload = json.dumps({
            "tag_name": TAG,
            "target_commitish": "master",
            "name": RELEASE_NAME,
            "body": body_text,
            "draft": False,
            "prerelease": False
        }).encode("utf-8")
        req = urllib.request.Request(create_url, data=payload, headers={**headers, "Content-Type": "application/json; charset=utf-8"})
        with urllib.request.urlopen(req) as resp:
            release = json.loads(resp.read().decode("utf-8"))
            print(f"  -> Created release ID: {release['id']}")
    else:
        raise

# 3. Upload Assets
release_id = release["id"]
upload_base_url = f"https://uploads.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases/{release_id}/assets"
existing_assets = {a["name"]: a for a in release.get("assets", [])}

assets = [
    {
        "name": "tzdlang-0.2.21.vsix",
        "path": os.path.join(REPO_ROOT, "dist", "tzdlang-0.2.21.vsix"),
        "type": "application/octet-stream"
    },
    {
        "name": "TzdSDK-v0.2.14-windows-x64.zip",
        "path": os.path.join(REPO_ROOT, "dist", "TzdSDK-v0.2.14-windows-x64.zip"),
        "type": "application/zip"
    },
    {
        "name": "TzdTools_Setup_v0.2.14.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.14.exe"),
        "type": "application/vnd.microsoft.portable-executable"
    },
    {
        "name": "TzdTools_Setup_v0.2.14_CPU.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.14_CPU.exe"),
        "type": "application/vnd.microsoft.portable-executable"
    }
]

print(f"[3/4] Uploading {len(assets)} release assets...")
for item in assets:
    name = item["name"]
    path = item["path"]
    mime = item["type"]

    if not os.path.isfile(path):
        print(f"  Warning: {path} not found, skipping.")
        continue

    file_size = os.path.getsize(path)
    size_mb = file_size / (1024 * 1024)

    # If already exists and complete, skip
    if name in existing_assets:
        ea = existing_assets[name]
        if ea.get("size") == file_size and ea.get("state") == "uploaded":
            print(f"  Asset {name} already uploaded ({size_mb:.2f} MB), skipping.")
            continue
        asset_id = ea["id"]
        print(f"  Deleting incomplete/previous asset ID {asset_id} for {name}...")
        del_url = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases/assets/{asset_id}"
        del_req = urllib.request.Request(del_url, headers=headers, method="DELETE")
        try:
            with urllib.request.urlopen(del_req) as resp:
                pass
        except Exception as e:
            print(f"    Delete error (ignored): {e}")

    print(f"  Uploading {name} ({size_mb:.2f} MB)...")

    # Use curl.exe for reliable streaming upload of large binaries
    upload_url = f"{upload_base_url}?name={urllib.parse.quote(name)}"
    curl_cmd = [
        "curl.exe",
        "-X", "POST",
        "-H", f"Authorization: Bearer {token}",
        "-H", f"Content-Type: {mime}",
        "-H", "Accept: application/vnd.github+json",
        "--data-binary", f"@{path}"
    ]
    if detected_proxy:
        curl_cmd.extend(["-x", detected_proxy])
    curl_cmd.extend(["--retry", "3", "--retry-delay", "5", "--progress-bar", upload_url])
    subprocess.run(curl_cmd, check=True)
    print(f"    -> Successfully uploaded {name}")

print("============================================================")
print(f"[4/4] Release {TAG} published successfully!")
print(f"URL: {release['html_url']}")
print("============================================================")

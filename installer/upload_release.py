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

TAG = "v0.2.13"
RELEASE_NAME = "TzdTools v0.2.13 - 核心动态库解耦 (tzd.dll)、C/C++ 原生 SDK、JVMTI 调试接口、TNI/Java 绑定与线程管理"
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
        body_text = """## 🚀 TzdTools v0.2.13 发布说明

### 💎 核心亮点

1. **核心运行时动态库解耦 (`tzd.dll` + `tzd.lib`)**：
   - 将 TzdLanguage 解释器、LLVM JIT 编译器、GC 垃圾回收器、NTT 大数运算引擎与 LibTorch/CUDA 深度学习运行时彻底解耦封装为独立的动态链接库 `tzd.dll` 与导入库 `tzd.lib`
   - CLI 宿主 `TzdTools.exe` 重构为轻量级加载器，仅依赖 `tzd.dll`
   - 提供工业级 C/C++ 头文件：`include/tzd/tzd.h` (纯 C API), `include/tzd/tzd.hpp` (现代 RAII C++ API), `include/tzd/tzd_export.h`

2. **JVMTI 风格高性能调试与观测接口 (`tzd_ti.h`)**：
   - 零性能损耗探针机制（未注册回调时分支开销为 0）
   - 支持方法进入/退出事件钩子（Method Entry / Exit Callbacks）
   - 支持线程生命周期事件通知（Thread Start / Thread End）
   - 新增类对象反射与全堆枚举：`tzd_ti_get_objects_by_class(const char* class_name)`，快速排查内存占用与活跃实例
   - 支持异常捕获与抛出事件监听（Exception Throw / Catch）

3. **类 JNI 原生接口 TNI 与官方 Java 绑定 (`tzd_tni.h` & `com.tzd.*`)**：
   - 提供完整的 TNI (Tzd Native Interface) 机制，支持原生 C/C++ 与宿主双向互调
   - 官方 Java 语言绑定 `tzd-java-sdk.jar` (`com.tzd.TzdVM`, `com.tzd.TzdInterpreter`, `com.tzd.TzdValue`)，Java 应用可无缝嵌入 TzdLanguage 执行复杂脚本与 AI 计算

4. **高级线程管理系统 API (`tzd_thread_*` & `tzd::ThreadManager`)**：
   - 线程全量堆栈快照追踪与转储（`tzd_thread_dump_all` / `dumpAllStackTraces`）
   - 自动化死锁检测算法（`tzd_thread_detect_deadlocks` / `detectDeadlocks`）
   - 线程审计日志管理与安全挂起/恢复控制（`tzd_thread_pause` / `tzd_thread_resume`）
   - 修复解释器销毁时的线程注册悬空指针异常，确保线程生命周期安全

5. **JIT 编译与神经网络模块参数绑定修复**：
   - 修复 PyTorch `Sequential` 模块 `registerModule` 在 JIT 编译时由于 Map 类型到指针类型转换导致的 Module verification failed 异常
   - 全面验证多卡与单卡 GPU 神经网络端到端训练与推理（`test_gpu_multi_card_training_and_inference.tzd`）

6. **标准化 SDK 分发构建流水线**：
   - 自动化打包流水线 `scripts/package_dist.bat`，包含标准清晰的工程分发目录：
     - `bin/`：包含可执行文件 `TzdTools.exe`、核心动态库 `tzd.dll`、运行依赖与标准库 `stdlib/`
     - `include/`：完整 C/C++ 对外头文件（`tzd.h`, `tzd.hpp`, `tzd_ti.h`, `tzd_tni.h`, `tzd_export.h`）
     - `lib/`：编译导入库 `tzd.lib`
     - `libs/`：外部依赖二进制包
     - `licenses/`：第三方开源软件授权许可文件
   - 支持一键打包 Windows 安装程序 `TzdTools_Setup_v0.2.13.exe` 与极简便携 SDK `TzdSDK-v0.2.13-windows-x64.zip`

---

### 📦 资产列表 (Release Assets)
- **TzdTools_Setup_v0.2.13.exe**：包含全套 CUDA 12.6 运行库、LibTorch GPU 运行时、编译器的 Windows 完整安装包 (1.4 GB)
- **TzdSDK-v0.2.13-windows-x64.zip**：面向 C/C++ 及 Java 开发者的高性能轻量 SDK（包含头文件、`tzd.lib`、`tzd.dll`、`tzd-java-sdk.jar` 及许可证）(16 MB)
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
        "name": "TzdSDK-v0.2.13-windows-x64.zip",
        "path": os.path.join(REPO_ROOT, "dist", "TzdSDK-v0.2.13-windows-x64.zip"),
        "type": "application/zip"
    },
    {
        "name": "TzdTools_Setup_v0.2.13.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.13.exe"),
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

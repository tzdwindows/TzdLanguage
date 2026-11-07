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

TAG = "v0.2.12"
RELEASE_NAME = "TzdTools v0.2.12 - 完整多线程系统、JIT 调用栈完整性修复与 LLVM IR 支配关系修复"
REPO_OWNER = "tzdwindows"
REPO_NAME = "TzdLanguage"
PROXY = "http://127.0.0.1:7897"

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Check proxy availability
def is_proxy_available(proxy_url):
    try:
        import socket
        parsed = urllib.parse.urlparse(proxy_url)
        with socket.create_connection((parsed.hostname, parsed.port), timeout=1.0):
            return True
    except Exception:
        return False

use_proxy = is_proxy_available(PROXY)
if use_proxy:
    print(f"  -> Using proxy {PROXY}")
    proxy_handler = urllib.request.ProxyHandler({'http': PROXY, 'https': PROXY})
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
        body_text = """## 🚀 TzdTools v0.2.12 发布说明

### 💎 核心亮点

1. **完整多线程系统新增入 stdlib**：
   - 线程邮箱直接通信（`Thread.send` / `Thread.recv`）
   - CSP Channel 管道（`Channel.send` / `Channel.recv` / `Channel.close`）
   - `Mutex` / `LockGuard` 互斥锁
   - `CondVar` / `Event` 条件变量与事件通知
   - `AtomicInt` / `AtomicBool` 无锁原子变量
   - 线程堆栈追踪（`Thread.getStackTrace` / `Thread.dumpAll`）
   - 线程调试（`Thread.pause` / `Thread.resume`）与死锁检测
   - 线程审计日志系统（CREATE / START / FINISH / PAUSE / RESUME / USER 事件）
   - `ThreadPool` 线程池任务分发

2. **JIT 调用栈完整性修复**：
   - 彻底移除 `jitTrackFrames()` 调试门阀，所有 JIT 函数（顶层调用、跨函数调用、类方法、递归）均无条件压栈
   - 修复 `rt_tzd_call_method` 与 `rt_call_sub_fast` 缺少 `pushCallStack` / `popCallStack` 的问题
   - 新增 `rt_get_call_stack_depth` / `rt_restore_call_stack_depth` 实现 try-catch 栈平衡

3. **LLVM IR 支配关系修复（`Instruction does not dominate all uses`）**：
   - 修复 try-catch 生成代码中 `stackDepth` 的读取指令未能支配 `catchBB` 导致的 LLVM 模块校验失败
   - 将 `rt_get_call_stack_depth()` 调用提前至 `setjmp` 之后的公共前驱块，使其严格支配 `try` 与 `catch` 两条路径

4. **AST 内联边界修复**：
   - 恢复 `tryInlineFunction` 对返回类型的严格守卫，禁止非纯数值函数被意外内联展开

---

### 📦 资产列表 (Release Assets)
- **TzdTools_Setup_v0.2.12.exe**：包含全套 CUDA 12.6 运行库、LibTorch GPU 运行时、编译器的 Windows 完整安装包
- **TzdTools_Setup_v0.2.12_CPU.exe**：轻量级 CPU 原生运行时安装包
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
existing_assets = {a["name"]: a["id"] for a in release.get("assets", [])}

assets = [
    {
        "name": "tzdlang-0.2.21.vsix",
        "path": os.path.join(REPO_ROOT, "dist", "tzdlang-0.2.21.vsix"),
        "type": "application/octet-stream"
    },
    {
        "name": "TzdTools_Setup_v0.2.12_CPU.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.12_CPU.exe"),
        "type": "application/vnd.microsoft.portable-executable"
    },
    {
        "name": "TzdTools_Setup_v0.2.12.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.12.exe"),
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

    size_mb = os.path.getsize(path) / (1024 * 1024)
    print(f"  Uploading {name} ({size_mb:.2f} MB)...")

    # If already exists, delete first
    if name in existing_assets:
        asset_id = existing_assets[name]
        print(f"    Deleting previous asset ID {asset_id}...")
        del_url = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases/assets/{asset_id}"
        del_req = urllib.request.Request(del_url, headers=headers, method="DELETE")
        try:
            with urllib.request.urlopen(del_req) as resp:
                pass
        except Exception as e:
            print(f"    Delete error (ignored): {e}")

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
    if use_proxy:
        curl_cmd.extend(["-x", PROXY])
    curl_cmd.extend(["--silent", "--show-error", upload_url])
    subprocess.run(curl_cmd, check=True)
    print(f"    -> Successfully uploaded {name}")

print("============================================================")
print(f"[4/4] Release {TAG} published successfully!")
print(f"URL: {release['html_url']}")
print("============================================================")

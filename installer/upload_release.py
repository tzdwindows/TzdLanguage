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

TAG = "v0.2.11"
RELEASE_NAME = "TzdTools v0.2.11 - 显式函数/Lambda返回类型特化、Lambda JIT 96,000x 内联跃升与 VSCode 扩展全面升级"
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
        body_text = """## 🚀 TzdTools v0.2.11 发布说明

### 💎 核心亮点与重大性能飞跃

1. **显式函数与 Lambda 返回类型标注支持 (`fun(...) -> int`)**：
   - 增加函数、类成员方法、原生方法及匿名 Lambda 表达式的显式返回类型语法（如 `fun fib(int n) -> int { ... }` 与 `var transform = fun(int val) -> int { ... };`）。
   - JIT 编译器在 Phase 1 阶段即可根据返回类型标注直接特化 native worker 原生浮点/整型寄存器通道，避免 AST 遍历类型猜测开销。

2. **Lambda 表达式 JIT 性能跃升 (96,000x 飞跃)**：
   - **Phase 1 作用域提升修复**：修复底层 AST 节点包装导致顶层带有 Lambda 赋值的变量无法在预扫描阶段注册为函数的缺陷，使 Lambda 能够完整参与全量 JIT 特化编译。
   - **AST 零开销直接内联 (`tryInlineFunction`)**：在调用循环内部实现 Lambda AST 零开销直接内联，彻底消除调用栈帧分配、闭包装箱与间接跳转开销。
   - **`FastCC` 原生 Native Worker 直通分发**：对于跨变量传递的 Lambda 实例，引入 `_worker_native` 专用寄存器直调通道（Fast Calling Convention）。
   - **消除栈帧穿透**：严格隔离 JIT Worker 与解释器 Entry 栈帧指针，彻底消除多层嵌套调用中的栈帧穿透与参数错位。
   - **基准实测**：在 200 万次 Lambda 紧凑调用（BENCH 4）测试中，执行时间从 **3539 ms 降至 0.036 ms**（36.8 微秒），实现近 **10 万倍性能提速**！

3. **类成员字段访问与空指针安全修复**：
   - 修复在 JIT 编译的成员方法中，未限定字段名直接赋值（如 `x = x + dx;`）导致误报 `尝试在空对象 (null) 上访问成员: 'x'` 的异常缺陷，建立针对当前实例 `this` 的直接字段插槽映射（`m_varFieldAllocas`）。200 万次类方法派发与字段更新仅需 **9.83 ms**！

4. **LLVM IR 浮点类型校验修复**：
   - 修复标准库 `time/DateTime.tzd` 在 `-O3` 深度优化下由于隐式类型提升触发的 LLVM IR 模块校验失败（`Both operands to a binary operator are not of the same type!`），实现全指令安全构建。

5. **VS Code 官方扩展 `tzdlang` (v0.2.21) 全面升级**：
   - 全面支持显式返回类型语法 `-> typeType` 的语法高亮着色。
   - 修复 LSP 语言服务器与诊断分析器在解析匿名 Lambda 参数（如 `fun(int val) -> int`）时错误报错“未声明的变量：'val'”的诊断 Bug。

---

### 📦 资产列表 (Release Assets)
- **TzdTools_Setup_v0.2.11.exe**：包含全套 CUDA 12.6 运行库、LibTorch GPU 运行时、编译器的 Windows 官方完整安装包
- **TzdTools_Setup_v0.2.11_CPU.exe**：轻量级 CPU 原生运行时安装包（仅 58 MB）
- **tzdlang-0.2.21.vsix**：升级版 VS Code 官方语言、语法高亮、LSP 智能感知与 DAP 交互断点调试插件
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
        "name": "TzdTools_Setup_v0.2.11_CPU.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.11_CPU.exe"),
        "type": "application/vnd.microsoft.portable-executable"
    },
    {
        "name": "TzdTools_Setup_v0.2.11.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.11.exe"),
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

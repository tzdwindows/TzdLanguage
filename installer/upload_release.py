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

TAG = "v0.2.10"
RELEASE_NAME = "TzdTools v0.2.10 - VSCode 插件全面升级、内置 94+ 本地函数注解、精准定义跳转与全局默认 JIT 极速引擎"
REPO_OWNER = "tzdwindows"
REPO_NAME = "TzdLanguage"
PROXY = "http://127.0.0.1:7897"

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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

# Set up urllib proxy
proxy_handler = urllib.request.ProxyHandler({'http': PROXY, 'https': PROXY})
opener = urllib.request.build_opener(proxy_handler)
urllib.request.install_opener(opener)

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
        body_text = """## 🚀 TzdTools v0.2.10 发布说明

### 💎 核心亮点与重大更新

1. **VS Code 官方扩展 `tzdlang` (v0.2.19) 体验飞跃**：
   - **内置 94+ 本地原生函数完整中文注解与签名文档**：在编辑器中悬停（Hover）或自动补全任何内置本地方法（如 `toString`, `print`, `input`, `len`, `range`, `abs`, `sqrt`, `time_now_sec` 等）时，自动呈现包含详细用途描述、完整类型签名、参数说明、返回值与可运行代码示例的丰富 Markdown 文档。
   - **修复本地函数跳转定义误入类方法**：修复在导入 `import "time/DateTime.tzd"` 等模块后，自由调用的原生函数 `toString` 被误跳转到类方法 `fun toString()` 的问题。在 LSP 作用域解析中增加了大括号层级隔离 (`braceDepth === 0`)，禁止将类内方法收录为全局函数，并在跳转前校验本地内置函数。
   - **全量关键字自动补全与代码块片段**：修复输入 `i` 无法补全 `if` 的缺陷，全面补全 `if`, `else`, `while`, `for`, `try`, `catch`, `throw`, `switch` 等关键字及带光标占位符的语句模板片段。
   - **全面修复默认 JIT 极速模式**：修复此前 VS Code 调试适配器因启动参数判定缺陷在未配置 `launch.json` 时误退化为 `--noJit` 纯树解释模式的严重性能问题，保证 F5 调试与普通运行均默认以 JIT 极速执行。

2. **REPL 控制台与底层控制台模式修复**：
   - 修复在交互终端输入 `import` 语句或调用 `input()` 时的无响应与阻塞问题。
   - 支持 Windows 控制台 Unicode 与中文输入法正常录入。
   - 调试器 DAP 协议支持端口冲突智能自动重试与多进程会话隔离。

3. **第二轮高抗优化基准（Round-2 Kernels）全线通过**：
   - 覆盖数值模拟循环（Monte Carlo）、字符串增长、哈希表、对象分配与 Lomuto 内存重排快排等抗优化场景，JIT 全套在 113ms 内急速通过。

---

### 📦 资产列表 (Release Assets)
- **TzdTools_Setup_v0.2.10.exe**：包含全套 CUDA 12.6 运行库、LibTorch GPU 运行时、编译器的 Windows 官方完整安装包
- **TzdTools_Setup_v0.2.10_CPU.exe**：轻量级 CPU 原生运行时安装包
- **tzdlang-0.2.19.vsix**：升级版 VS Code 官方语言、语法高亮、LSP 与 DAP 交互断点调试插件
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
        "name": "tzdlang-0.2.19.vsix",
        "path": os.path.join(REPO_ROOT, "dist", "tzdlang-0.2.19.vsix"),
        "type": "application/octet-stream"
    },
    {
        "name": "TzdTools_Setup_v0.2.10_CPU.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.10_CPU.exe"),
        "type": "application/vnd.microsoft.portable-executable"
    },
    {
        "name": "TzdTools_Setup_v0.2.10.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.10.exe"),
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
        "--data-binary", f"@{path}",
        "-x", PROXY,
        "--silent",
        "--show-error",
        upload_url
    ]
    subprocess.run(curl_cmd, check=True)
    print(f"    -> Successfully uploaded {name}")

print("============================================================")
print(f"[4/4] Release {TAG} published successfully!")
print(f"URL: {release['html_url']}")
print("============================================================")

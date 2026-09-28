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

TAG = "v0.2.9"
RELEASE_NAME = "TzdTools v0.2.9 - PyTorch 真实深度模型训练自检、单卡与多卡 DataParallel 训练推理闭环及 JIT 零泄漏优化"
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
        body_text = """## 🚀 TzdTools v0.2.9 发布说明

### 💎 核心亮点与突破性进展

1. **JIT 编译引擎与解释器训练循环内存泄漏彻底根除 (Zero Memory Leaks)**：
   - 彻底修复 JIT 方法调用与函数调用中未对参数插槽执行析构释放的内存泄漏问题（引入就地析构与就地构造初始化）。
   - 修复函数栈退出时全局 JIT 内存及垃圾收集器未被清理的问题，并在 `torch_gc()` 中深度挂接内存清空。
   - 100 次完整反向传播训练循环张量驻留数从数百个暴增彻底稳定为 **绝对零泄漏**（初始 4 张量 -> 100 轮后垃圾回收仍为精确 4 张量）。

2. **完整 7 大真实深度模型预飞行自检测试套件 (Pre-Flight Self-Check Suite)**：
   - 包含 Autograd 梯度精度、Deep MLP (XOR 非线性逼近收敛)、CNN 视觉管线 (Conv2d + BatchNorm + Pooling)、时序循环网络 LSTM (多步 BPTT 反向传播)、Transformer 多头自注意力 MultiheadAttention、模型检查点 StateDict 保存/恢复 100% 位精确一致性、100 次迭代张量生命周期稳定性自检。全部 100% 通过。

3. **单卡 (cuda:0, NVIDIA P106-090) 真实模型训练到底与推理验证**：
   - 全硬件探测与 CUDA 显存状态监控。
   - 深度网络完整参数与缓存自动迁移至 GPU (`.to("cuda:0")`)。
   - 真实非线性回归任务在 GPU 上训练 70 轮，Loss 从 3.738 收敛至 0.0087（收敛精度达到 0.01 级别），GPU 推理预测与目标值精准吻合。

4. **多卡 / 分布式 DataParallel 训练与跨卡高吞吐并行推理 (Parallel Inference)**：
   - `DataParallel` 支持多设备分片切分 (`scatter`)、多路前向计算与聚合输出 (`gather`)。
   - 集合通信 `AllReduce` (mean / sum) 与分布式数据并行 `DistributedDataParallel` (DDP) 跨设备梯度同步与训练收敛。
   - 提供 `parallelInference(model, input, devices)` 接口，跨多卡/多设备高吞吐并行推理，批处理自动按设备切分还原。

5. **全套标准库与 VSCode 插件同步升级**：
   - 同步升级官方 VSCode 插件 `tzdlang` 至 **v0.2.18**。
   - 发布完整的 GPU 版与 CPU 版 Windows 安装程序。

---

### 📦 资产列表 (Release Assets)
- **TzdTools_Setup_v0.2.9.exe**：包含全套 CUDA 12.6 运行库、LibTorch GPU 运行时、编译器的 Windows 官方 GPU 完整安装包 (1.33 GB)
- **TzdTools_Setup_v0.2.9_CPU.exe**：轻量级 CPU 原生运行时安装包 (56 MB)
- **tzdlang-0.2.18.vsix**：VSCode 官方语言、语法高亮、LSP 与 DAP 交互断点调试插件
"""
        payload = json.dumps({
            "tag_name": TAG,
            "target_commitish": "main",
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
        "name": "tzdlang-0.2.18.vsix",
        "path": os.path.join(REPO_ROOT, "vscodePlugin", "tzdlang", "tzdlang-0.2.18.vsix"),
        "type": "application/octet-stream"
    },
    {
        "name": "TzdTools_Setup_v0.2.9_CPU.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.9_CPU.exe"),
        "type": "application/vnd.microsoft.portable-executable"
    },
    {
        "name": "TzdTools_Setup_v0.2.9.exe",
        "path": os.path.join(REPO_ROOT, "dist", "TzdTools_Setup_v0.2.9.exe"),
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

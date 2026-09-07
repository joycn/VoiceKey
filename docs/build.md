# 构建与测试

## 工具链

- ESP-IDF **v5.5.2**，target **esp32s3**。
- Python **3.11**；系统自带 Python 若过旧，使用独立 Python 环境。
- CMake **3.30.9**、Ninja **1.13.2**。
- 组件直接版本见各 `idf_component.yml`，传递依赖以 `firmware/dependencies.lock` 为准。

首次安装需要访问官方 GitHub、Python 包源和 ESP Component Registry。模型在构建期打包到 Flash；设备运行时无需下载模型或访问网络。

```bash
git clone --branch v5.5.2 --depth 1 --recursive --shallow-submodules \
  https://github.com/espressif/esp-idf.git ~/esp/esp-idf-v5.5.2
# 确保 python3 指向 Python 3.11，然后安装 ESP32-S3 工具链：
~/esp/esp-idf-v5.5.2/install.sh esp32s3
. ~/esp/esp-idf-v5.5.2/export.sh
python -m pip install cmake==3.30.9 ninja==1.13.2
```

`pip` 此时必须运行在 ESP-IDF 创建的虚拟环境。项目 `scripts/build.sh` 默认查找上述 IDF 路径，也接受 `IDF_PATH`；可通过 `VOICEKEY_PYTHON` 指定 Python 3.11 可执行文件。安装在 uv 下的 Python 3.11 会被脚本自动发现。

本机在代理环境中遇到 pip 的 SOCKS 依赖/连接池错误。工具链下载和 IDF 虚拟环境创建完成后，使用 uv 在同一个虚拟环境补齐依赖，保留 IDF 的版本约束：

```bash
uv pip install --python ~/.espressif/python_env/idf5.5_py3.11_env/bin/python \
  --default-index https://pypi.org/simple PySocks cmake==3.30.9 ninja==1.13.2
uv pip install --python ~/.espressif/python_env/idf5.5_py3.11_env/bin/python \
  --default-index https://pypi.org/simple \
  -r ~/esp/esp-idf-v5.5.2/tools/requirements/requirements.core.txt \
  -c ~/.espressif/espidf.constraints.v5.5.txt
```

## 构建

在项目根目录执行：

```bash
./scripts/build.sh
./scripts/test.sh
./scripts/test-descriptors.sh
./scripts/test-usb.sh
```

输出位于 `firmware/build/`：`voicekey.bin`、bootloader、分区表、模型分区镜像、`flasher_args.json` 和 ELF。不要只烧应用镜像而遗漏模型分区。

默认配置是 16 MB Flash、8 MB Octal PSRAM；必须与实物匹配。`./scripts/build.sh menuconfig` 可调整 VoiceKey 的 PCM 声道、增益、触发冷却和开发用 VID/PID。修改默认配置不会自动覆盖已有 `sdkconfig`；复现实验应记录实际配置。

默认启用 ESP-SR 的 Hi ESP 模型。更换唤醒词需选择真实受支持模型并重新生成模型分区；修改字符串不会改变模型识别的词。启动日志会输出模型和词。

## 自动测试

核心测试直接编译固件使用的 `voicekey_core.c`，覆盖转换、环形缓存、独立消费者、静音/恢复、USB 状态、HID 脉冲与过期推理；随机测试与独立参考队列比较 50,000 次操作。

描述符测试直接编译实际 `usb_descriptors.c`，使用下载的固定 TinyUSB 头文件，检查接口数、UAC2 拓扑、端点、16-bit 单声道格式、控制声明和字符串。这不是实际 macOS 枚举测试。

USB 回调测试直接编译 `usb_device.c` 与核心状态机，以固定 TinyUSB 头文件校验接口契约；仅替代 IDF 调度和 USB 传输。覆盖采样率/静音请求、无效控制值、物理静音优先、音频包、流关闭、暂停/恢复、HID 发送失败重试及事件过期。它不运行 USB 控制器，也不代替主机枚举和 FreeRTOS 实时调度测试。固件对 USB 组件启用 `-Werror=missing-prototypes`，防止回调拼写错误被当成未使用函数静默链接。

默认启用 UndefinedBehaviorSanitizer。在支持 AddressSanitizer 的工具链上可执行：

```bash
SANITIZERS=address,undefined ./scripts/test.sh
SANITIZERS=address,undefined ./scripts/test-descriptors.sh
```

当前 Mac 的系统 AddressSanitizer 在空程序启动时也崩溃，故本机不能提供 ASan 通过证据。无硬件测试不验证 FreeRTOS 调度、电气连接、音频质量或远场指标。

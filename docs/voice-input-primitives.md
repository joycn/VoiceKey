# VoiceKey 语音输入原子能力

本功能提供固件 C API 与实验开关，面向固定位置多人交互。没有游戏引擎、HA、座位按钮硬件或远程配置界面；也不通过 USB 音频假装传递玩家身份。当前板上固件不会因单纯代码修改而改变。

## 能力列表

| ID | 原子能力 | 实现入口 | 可验证边界 |
|---|---|---|---|
| A01 | 语音活动观测 | `vk_observation_t.speech` / `vk_xvf_direction` | 使用 XVF 的语音标志；不是身份识别 |
| A02 | 带时间戳方向观测 | `vk_xvf_direction` / `vk_observation_fresh` | 固件1.0.8；100ms有效期；错误独立标记 |
| A03 | 坐标校准 | `vk_angle_calibrate` / `config.offset` | 原始角度加偏移，归一化0..359；实物正前方需校准 |
| A04 | 区域映射 | `vk_region_map` | 最多8区域，跨0度、边界含端点，重叠返回unknown |
| A05 | 稳定方向获取 | `vk_spatial_observe`，FIRST策略 | 多次新观测连续满足；250ms默认确认，固定候选锚点±20° |
| A06 | 关键词检测 | 现有 `wake_engine.c` | 保留“你好小智”；不新增抢麦词模型 |
| A07 | 事件来源关联 | `vk_event_attribute` / runtime listener epoch | 明确时间区间可查询64条历史；不足、跨区域、断档或无语音返回unknown |
| A08 | 发言权 | `vk_floor_grant/request/release/revoke` | grant显式替换；request由preempt决定是否抢占；generation每次授予/撤销变化 |
| A09 | 音频源能力查询 | `vk_source_supported` | 仅支持现有AUTO_PROCESSED；FIXED_DIRECTION明确不支持，未实现定向分离 |
| A10 | 音频门控 | `vk_spatial_gate` / `vk_set_output_gate` | 输出零样本；全局注册区域唤醒监听独立；无效/物理静音优先 |
| A11 | 状态与诊断 | `vk_runtime_spatial_status` / `scripts/diagnose.py` | 角度、年龄、区域、owner、floor/route代次、listener、拒绝原因 |

公共类型和纯逻辑见 `firmware/components/voicekey_core/include/voicekey_spatial.h`；运行时适配 API 见 `firmware/main/runtime.h`。纯逻辑无堆分配，调用者负责串行化；runtime API 内部加锁，配置只在任务上下文调用。

## 组合与调用

默认 `CONFIG_VOICEKEY_SPATIAL` 关闭，保持原有拾音且不额外读取DOA。开启后是单区域设备坐标0°±30°、FIRST模式。不要把设备坐标0°直接当作外壳正前方。可通过正常 ESP-IDF 配置界面开启，随后使用项目构建脚本；没有自动烧录。

在应用初始化完成后用 C API 设置多人布局，示例为四个座位，每座位±20°，间隔90°：

```c
vk_spatial_config_t c = vk_spatial_defaults();
c.enabled = true;
c.policy = VK_FLOOR_MANUAL;
c.count = 4;
c.offset = 0; /* 根据实测校准 */
for (unsigned i = 0; i < c.count; ++i)
    c.regions[i] = (vk_region_t){ .center = i * 90, .half_width = 20 };
if (!vk_runtime_voice_configure(&c)) { /* 配置未应用，旧配置不变 */ }
/* 等待有效的板状态、时钟和方向观测后，调度器选择座位1： */
if (!vk_runtime_floor_request(1, true)) { /* 不满足条件，稍后重试 */ }
/* 座位1完成回答： */
vk_runtime_floor_release(1);
```

座位编号是0起始区域索引。`request(region,false)` 仅在无owner或同一owner时成功；`true`允许替换。不存在自动轮次、队列或主持人优先级，上层自行组合。配置变化会清除旧发言权、音频和监听上下文，不保留旧区域索引。运行时配置只保存在内存，重启恢复编译默认；上层需在启动时重新应用座位配置。

只读观察模式：`c.enabled=true; c.observe_only=true;` 后应用配置，会读取/报告方向但保持原始USB及唤醒流，不接受发言权请求。适合先验证方向数据；数据失效仅标记诊断，不启动空间拒绝，物理静音和原有安全检查仍然生效。

三种发言策略（`observe_only=false`）：

- **MANUAL**：上层指定座位，停顿不释放；显式release或安全故障才清除。
- **FIRST**：第一个连续稳定方向获锁，随后要求区域和锁角度同时匹配；最后匹配语音后2秒释放。其他方向不能续租；无语音时保留锁但音频门关闭。
- **KEYWORD**：全局监听已登记区域。首次可归属唤醒取得发言权并沿用原快捷键；已有owner时的有效唤醒只更新发言权，不反复触发ChatGPT快捷键。它不是ChatGPT会话状态同步：断开/静音后重新取得发言权的快捷键行为仍需应用实测。

先配置不同区域，再检测关键词，不需要先获得输出发言权。设备仅有一个当前自动选束音频流；“全局监听”意为不按owner过滤WakeNet，并不意味着每人有一个独立可识别音轨。

## 时间与失效规则

```text
XVF DOA/speech -> 校准 -> 区域 -> 方向/发言权策略 -> USB输出门
        |                    |
        +-> 来源上下文 ------+-> 全局WakeNet -> 可归属关键词请求
物理静音/采集故障 -------------> 关闭两路；播放沿用原有独立控制
```

- DOA在I2C事务前记时，方向读取失败不会冒充物理静音或引发XMOS重配置；100ms过期关闭空间门。方向读放在原有安全轮询前，不把旧安全状态标成新鲜。
- `floor generation` 表示发言权变更；`route generation` 表示输出许可/来源变更。采集开始前取得route token，提交时再次核对。旧块拒绝，切换清USB队列及FIR历史，排空3个RX描述符加当前填充块；不因普通方向变化重启I2S。
- 唤醒流按持续有效的来源区域绑定epoch；发言权切换、新的语音区域或无效观测使旧队列/推理失效。静音段送零给监听器，不把未分类声音送入已有来源上下文。
- WakeNet不提供精确词起止时间，因此内置接法使用保守的同区域输入上下文和最短稳定时间，不能证明物理词源。外部有事件起止时间时可调用历史区间归属API；时间覆盖不足明确返回unknown。
- 物理静音/采集不安全优先，清发言权。方向门关闭时仍保持I2C观测与USB枚举。已送达USB主机的在途样本无法撤回。
- 本版没有确认前音频回放，会有句首截断风险。所有策略保持原设备语音带宽、自动选束和AEC参考路径。

## 诊断

当前固件输出schema5、384字节；原schema3/4字段位置保留，空间数据仍在192–223字节。更新后的 `python3 scripts/diagnose.py --samples 100 --interval 0.1` 仍可解码旧schema3固件。旧版诊断程序不能解码新固件，应配套更新。

重点看 `spatial_enabled`、`observe_only`、`direction_valid`、`direction_age_ms`、`direction_region`、`floor_owner`、`floor_generation`、`route_generation`、`listener_region`、`output_allowed` 和 `output_reason`。`output_allowed`表示本地采集安全与空间门许可，主机静音、USB连接/stream状态仍需结合原有字段检查；它不是实际声能或已发送包数。语音原子接口不把这些字段插进USB音频，也不自动发送给ChatGPT。

## 实验与量化验收

以下是待实测的建议门槛，不是通过结果。先旁路、仅观察，再单扇区，再多座位/抢麦；每组保存固件身份、参数、座位角度、距离、音量、原始USB录音、带时间的遥测。固定播放语料用于可重复对照，人声用于体验验证。

| 项目 | 方法 | 建议目标 |
|---|---|---|
| 区内保留 | 0°/±15°各30句，与同条件旁路对照 | 关键词保留率≥95%；记录句首损失 |
| 区外拒绝 | ±60°/90°/180°各30句单声源 | 对齐语音窗口能量降低≥20dB，完整可懂句漏入≤1/30 |
| 边界 | ±30°/±35°轻微转头 | 单列开关次数/误拒，不混入中心成绩 |
| 首句延迟 | 声源到USB有效语音时间对齐 | 门控相对旁路新增P95≤400ms |
| 发言归属 | 四座位轮流及按钮/API抢麦各30轮 | 接口指定座位与owner一致；旧音频不串入新轮次 |
| 语音抢麦 | 他人说话中、设备播放中各30轮 | 报告成功次数、错误来源、耗时分布；不承诺重叠时必成功 |
| 方向锁 | 两人轮流/插话各30轮 | 有效锁期间错误换锁0/30；2s释放误差≤100ms |
| 软件安全 | 失效方向/读失败/物理静音/延迟推理 | 新USB采样为零、无旧推理触发，播放保持独立 |
| 双讲 | 两人固定位置，交换音量/先后 | 报告目标关键词保留、干扰关键词漏入、误闭时长；无隔离达标声明 |

BUG-001静音后低音量仍未解决。前后同声源基线漂移>3dB、削波或复现原问题时，保留该组但标记受干扰，不用它证明门控抑制成功。不要用短窗I2C正常、LED正常或编译通过替代声学验收。

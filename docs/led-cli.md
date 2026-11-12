# VoiceKey LED CLI：座位与麦权反馈

从项目目录通过 XIAO USB-C 控制灯环，不需要 HA、MQTT 或额外串口。LED 操作不授予麦权、不修改收音配置。完整功能需 ESP 协议版本1.6；旧1.4支持简单颜色，1.5支持全参数。设备与主机脚本应一起更新，脚本保留schema3/4诊断兼容。

```bash
python3 -m venv .venv
.venv/bin/python -m pip install hidapi
.venv/bin/python scripts/voicekey.py led status
```

可选全局参数 `--vid 0xcafe --pid 0x4014` 放在 `led` 前。Mac共享打开HID，不独占键盘；匹配零个或多个设备均报错。

## 基础参数

```bash
.venv/bin/python scripts/voicekey.py led red
.venv/bin/python scripts/voicekey.py led '#00FF80'
.venv/bin/python scripts/voicekey.py led off
.venv/bin/python scripts/voicekey.py led --effect breathe --color blue --brightness 30 --speed 1 --gamma 0
.venv/bin/python scripts/voicekey.py led --brightness 60
.venv/bin/python scripts/voicekey.py led --effect doa --doa-colors black blue
.venv/bin/python scripts/voicekey.py led --effect ring --ring-colors red green blue white off off off off off off off off
.venv/bin/python scripts/voicekey.py led auto
```

- effect：off/0、breathe/1、rainbow/2、solid/3、doa/4、ring/5。
- brightness、speed：原始uint8值0..255；Gamma0关闭、1开启。速度不是秒或Hz，不同灯效对速度/亮度参数的使用不同，视觉效果需实测。
- color：六位RGB或red/green/blue/white/yellow/cyan/magenta/purple/orange/black/off。`#`需引号。
- doa-colors：基底色、方向色；ring-colors：按物理索引0..11给出12个RGB。
- 选项形式只修改指定的手动参数，其他参数保留。未启用对应灯效的DOA/ring颜色保存在请求配置中，到选择该灯效时写入并读回。
- 简写 `led red/off` 延续旧行为，设置静态/关闭效果，亮度30、速度1、Gamma0。`auto`清除手动和临时效果。

参数依据[固定版本官方命令表](https://github.com/respeaker/reSpeaker_XVF3800_USB_4MIC_ARRAY/blob/a652fe79da3a292b25decc0e1e7f267d29bb0284/python_control/xvf_host.py)。灯电源GPIO属于板级安全控制，不作为任意GPIO命令开放。

## 校准与座位布局

```bash
.venv/bin/python scripts/voicekey.py led identify --index 0 --duration-ms 3000
.venv/bin/python scripts/voicekey.py led identify --index 1 --duration-ms 3000
.venv/bin/python scripts/voicekey.py led layout apply examples/led-seats.json
.venv/bin/python scripts/voicekey.py led preview --seat A --state owner --duration-ms 3000
.venv/bin/python scripts/voicekey.py led preview --seat B --state receiving --duration-ms 3000
.venv/bin/python scripts/voicekey.py led preview --seat B --state granted --duration-ms 1000
.venv/bin/python scripts/voicekey.py led preview --seat B --state denied --duration-ms 1000
```

布局格式见[四座位示例](../examples/led-seats.json)。先观察0号和1号灯的物理位置，填写0号对应的 `zero_degrees` 及索引递增方向 `clockwise/counterclockwise`。角度必须与语音区域的校准坐标一致；示例0°不是已校准的实物正前方。

每座位包括唯一名称（1..32字符）、唯一语音区域编号region0..7、中心角度0..359、完整灯弧宽度1..360度。最多8座位。灯弧包含边界；没有灯珠落入范围时选择最近灯珠，等距取低索引。任何座位共享灯珠均拒绝整份布局，保留原布局。

设备只保存区域映射，不存名字，不改语音区域。成功应用后，CLI将JSON存到 `~/.local/share/voicekey/led-layout-<vid>-<pid>.json`。预览检查本地布局指纹与设备一致；也可用 `--layout <文件>`。设备重启后必须重新apply；没有NVS持久化。

预览只模拟灯光，不意味着真实麦权或收音已经改变。duration范围100..60000ms，计时在设备端；命令行退出不影响计时。新的identify/preview替换旧的，auto取消；到期和USB断连清除临时效果，回到最新状态。

## 自动状态

优先级：故障 > 物理静音 > 限时校准/演示 > 手动参数 > 真实授予/拒绝事件 > 真实麦权/收音 > 原待机灯效。

- 持有麦权：对应灯弧蓝色强度15；暂停说话不转移方向。
- 本地语音通过门控且USB麦克风流运行、主机未静音：蓝色强度30。
- 授予麦权：蓝色双闪；已知座位的明确拒绝：琥珀色双闪。未知来源不猜座位。
- 空间功能未启用、方向状态无效或没有对应布局时，不显示真实座位收音。
- `led auto`仅启用灯光跟随；不会打开空间收音功能。灯光不代表ChatGPT已准备好接收。

设备当前空间功能默认关闭，因此部署后可验证布局和预览；真实麦权显示需在现有语音配置启用后另行验收。

## 结果与读回

新版命令使用随机请求编号，等待最多3秒获取匹配结果。`received`已接收待执行；`applied`已应用配置或完成当前帧I²C写入与读回；`covered`被更高优先级遮盖；`failed`非法请求或I²C错误；`expired`已到期/断连。超时或被其他客户端替换返回错误，可用 `led status`检查最新请求。

状态提供mode、source、target_region、remaining_ms、reason、manual_config、applied_config及layout_token。config数组为effect/brightness/speed/gamma、RGB、DOA两组RGB、ring12组RGB。manual_config是请求配置；applied_config是上次实际帧的配置，只有当前灯效相关的颜色被写入和验证。Gamma保留策略用gamma_preserved标注；不是连续RGB回读，也不是肉眼颜色确认。非法命令的失败不清除已有有效灯光。

## USB协议

无新增接口。Feature Report从224扩展到384字节，描述符使用16位report count；GET_REPORT返回schema5，保留0..223字段。hidapi前置ID0，总长度385。

v3写入：0..3为VKLC，4为3，5为操作码，6..9为非零请求编号LE32，其后为操作体，剩余字节必须零：

| 操作 | 操作体 |
|---|---|
| 1 auto | 无 |
| 2 patch | mask1字节＋49字节config；未选字段须零 |
| 3 layout | count1＋zero角度LE16＋reverse1＋8条(region1/centerLE16/widthLE16)＋tokenLE32；未使用条目为零 |
| 4 identify | index1＋durationLE32 |
| 5 preview | region1＋state1(owner0/receiving1/granted2/denied3)＋durationLE32 |

mask各bit依次effect、brightness、speed、gamma、color、DOA colors、ring colors。旧v1/v2在新报告长度下仍可解析；老host脚本应升级，不承诺旧脚本适配新描述符。

schema5扩展：224请求编号；228结果；229模式；230来源；231区域+1；232剩余ms；236原因；237applied有效；238座位数；239预览状态；240..288应用帧；289Gamma保留；292布局token；296..311按region排列的8个LED mask；328..376手动配置。其余保留。

## 验收

自动测试覆盖协议、布局、超时、覆盖、真实状态映射及I²C读回；不代替上板验证。实机分别记录方向对应、视觉颜色、静音按钮优先级、录放音并行。固件只从XIAO接口部署，不刷XVF3800。灯环动画变化最多10帧/秒；相同帧跳过，安全提示立即优先。

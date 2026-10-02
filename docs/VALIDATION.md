# 测试与验证指南

本文档介绍 Endfield-UidMask（身份显示伪装模组）的自动化测试套件与功能验证方法。

---

## 自动化测试

项目包含 Native C++ 文本置换测试与 Lua 热重绘触发测试：

```powershell
ctest --test-dir build -C Release --output-on-failure
```

### 测试套件说明

- **CoreTests (`tests/core_tests.cpp`)**：
  - 测试 UID 数字序列、玩家昵称与 `#` 编号的匹配识别算法。
  - 边界用例测试：超长文本截断、空别名、非 ASCII 字符处理、部分匹配防误伤逻辑。

- **HookTests (`tests/hook_tests.cpp`)**：
  - 基于合成 IL2CPP 环境测试 `PopulateTextProcessingArray` 底层 Hook。
  - 验证文本处理期间的临时渲染 Buffer 交换与函数退出时的原始 Buffer 准确恢复。
  - 异常展开时的内存安全验证。

- **ManagedFixture (`tests/managed_fixture.cpp`)**：
  - 模拟 Unity TextMeshPro 数据结构与字符数组。
  - 校验 UTF-8 与 UTF-16 编码转换以及不同布局下的边界安全。

- **LuaTests (`tests/lua_tests.py` / `tests/ui_mock.lua`)**：
  - 测试模组配置变更时的 ZML 订阅事件分发。
  - 验证游戏场景中处于激活状态的 TMP 控件的强制重绘触发逻辑。

---

## 实机功能验证清单

在游戏内测试时，可按照以下流程逐项验证：

1. **UID 伪装验证**：
   - 观察主界面左下角 HUD、ESC 菜单底栏与个人名片上的 UID，确认是否已替换为配置的别名。
   - 在模组设置中更改别名（例如改为 `88888888`），返回游戏确认文字是否即时刷新。

2. **玩家昵称与 # 编号伪装**：
   - 开启「伪装玩家昵称」与「伪装昵称 # 编号」，在名片界面观察自身昵称与后缀编号是否正确替换。
   - 打开聊天频道或他人名片，确认普通正文与其他玩家信息未被错误替换。

3. **开关与还原**：
   - 在模组设置中关闭各项伪装开关，确认界面显示立即恢复原生真实信息。

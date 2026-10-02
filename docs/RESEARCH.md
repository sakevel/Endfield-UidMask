# 客户端文本渲染与 UID 遮罩机制

本文档说明游戏界面 UID 渲染管线、TextMeshPro 底层缓冲区拦截机制及托管内存契约。

---

## 1. 游戏内 UID 展示链路

终末地在多个界面场景下显示玩家的 UID：
- **主界面 (HUD)**：`UI/Panels/UIDPanel/UIDPanelCtrl`，读取 `playerInfoSystem.roleId` 渲染到左下角；
- **手表与个人名片**：`BusinessCardPersonalInfoNode`，在个人名片节点展示 `platformRoleId`，名片复制功能直接读取文本控件内容；
- **截图与水印**：`BottomNodeWatermarkUI` 渲染当前角色 UID 水印。

若直接修改逻辑层的 `roleId`，会影响好友查询与身份校验；若仅 Hook UI 层的文本 Setter，又可能破坏复制功能与格式排版。因此模组选择在 TextMeshPro 底层字符排版管线实施渲染拦截。

---

## 2. TextMeshPro 底层拦截机制

### 2.1 拦截切入点
- 核心渲染准备函数：`Unity.TextMeshPro.dll` 中的 `TMP_Text.PopulateTextProcessingArray`。
- 文本后备容器：`m_TextBackingArray`（嵌套值类型 `TextBackingContainer`），包含：
  - `m_Array`: `System.UInt32[]`（Unicode 代码点数组）；
  - `m_Count`: `System.Int32`（有效字符数）。

### 2.2 缓冲区安全置换 (Buffer Swap)
- 在 `PopulateTextProcessingArray` 执行期间，临时将后备数组置换为包含自定义别名文本的 `UInt32` 代码点数组。
- 排版与顶点生成完毕后，立即利用 RAII 守卫复原原始数组引用与计数。
- 此方案确保：
  - 界面上呈现自定义别名或掩码文本；
  - 业务层逻辑、原始属性及剪贴板复制依然保持原生一致性。

---

## 3. IL2CPP 托管内存与引用契约

- 针对托管数组引用的读写，调用 `il2cpp_field_set_value` 时直接传递数组对象本身的引用，避免通过指针间接寻址造成引用悬空。
- 别名配置修改后通过公共接口实时通知相关文本控件重刷排版缓存，实现配置热生效。

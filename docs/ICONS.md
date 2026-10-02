# 图标设计与导出说明

本文档说明 Endfield-UidMask 的图标规范与导出流程。

---

## 图标规范

- **模组图标** (`mod/icon.png`)：
  256×256 透明 RGBA PNG，遵循终末地工业几何风格设计（灰白分层 + 亮黄点缀色）。
  色板包含：`#313131` / `#808080` / `#b4b4b4` / `#ffffff` / `#ffef00`。

---

## 导出流程

从原始矢量/高分位图生成发布所用的轻量图标：

```powershell
./tools/export-icon.ps1 -Source assets/icon-source.png -Destination mod/icon.png
```

# DeaDBeeF Vinyl Record 插件

[DeaDBeeF](http://deadbeef.sourceforge.net) 音频播放器插件，将专辑封面渲染为逼真的旋转黑胶唱片。

音乐播放时唱片旋转，暂停/停止时静止。专辑封面显示在唱片中央。

## 功能特性

- 逼真的黑胶唱片渲染，包含同心圆纹理和反光效果
- 播放时平滑旋转动画
- 多策略专辑封面获取（内嵌 ID3v2、artwork 插件、目录扫描）
- 可通过插件设置调整控件大小和封面区域比例
- 右键菜单

## 截图

*(在此添加截图)*

## 环境要求

- DeaDBeeF 1.10+
- GTK+ 3
- Cairo
- GdkPixbuf

## 编译

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Windows

输出 `vinyl_record.dll`，需要 GTK3 开发库。

### Linux

输出 `vinyl_record.so`。

```bash
# 安装依赖（Debian/Ubuntu）
sudo apt install libgtk-3-dev cmake gcc
```

## 安装

将编译好的插件文件（`vinyl_record.dll` 或 `vinyl_record.so`）复制到 DeaDBeeF 的插件目录，然后在 **编辑 > 首选项 > 插件** 中启用。在设计模式布局中添加"Vinyl Record"控件即可。

## 许可证

MIT License - Copyright (c) 2026 Jiangxing Zhao

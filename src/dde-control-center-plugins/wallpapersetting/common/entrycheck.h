// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ENTRYCHECK_H
#define ENTRYCHECK_H

#include <QDebug>
#include <QGSettings>

namespace ddplugin_desktop_util {

// 壁纸入口显隐管控，与 screensavercheck.h 的 show-screen-saver 检查对位；
// key 不存在（主仓库包较老）时保持显示，即主线默认行为
static inline bool wallpaperEntryVisible()
{
    QGSettings desktopSettings("com.deepin.dde.filemanager.desktop", "/com/deepin/dde/filemanager/desktop/");
    if (desktopSettings.keys().contains("showWallpaper") && !desktopSettings.get("showWallpaper").toBool()) {
        qWarning() << "Gsetting show-wallpaper is false";
        return false;
    }

    return true;
}

}
#endif // ENTRYCHECK_H

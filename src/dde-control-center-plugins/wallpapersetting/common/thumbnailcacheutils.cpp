// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnailcacheutils.h"

#include <QFile>
#include <QIODevice>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QDebug> 

namespace ThumbnailCacheUtils {

QString calculateFileHash(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for hashing:" << filePath;
        return QString(); // 返回空 QString 表示失败
    }
    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!file.atEnd()) { // 优化：如果文件为空，不需要读取
         constexpr qint64 bufferSize = 8192;
         while (!file.atEnd()) {
             hash.addData(file.read(bufferSize));
         }
    }
    file.close();
    return QString::fromLatin1(hash.result().toHex()); // 明确使用 fromLatin1
}

QString getThumbnailCachePath(const QString &originalPath, const QSize &size) {
    // 优先使用 CacheLocation，如果不可写，尝试回退到 GenericCacheLocation
    QString cacheBaseDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheBaseDir.isEmpty()) {
         cacheBaseDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
         if(cacheBaseDir.isEmpty()){
             qWarning() << "Cannot find writable cache location!";
             // 在极端情况下，可能返回一个无效路径，调用者需要处理
             return QString();
         }
    }

    QString cacheDir = cacheBaseDir + QDir::separator() + THUMBNAIL_CACHE_DIR_NAME;
    QDir dir; // 创建 QDir 对象来处理路径创建
    if (!dir.exists(cacheDir)) {
        if (!dir.mkpath(cacheDir)) {
             qWarning() << "Failed to create thumbnail cache directory:" << cacheDir;
             // 即使创建失败也继续，后续保存会失败，但尝试加载可能仍然有用（如果目录已存在但权限有问题）
        }
    }

    QString hash = calculateFileHash(originalPath);
    if (hash.isEmpty()) {
        // 如果计算哈希失败，使用路径本身的哈希作为备选
        qWarning() << "Failed to calculate file hash for:" << originalPath << "Using path hash as fallback.";
        hash = QString::fromLatin1(QCryptographicHash::hash(originalPath.toUtf8(), QCryptographicHash::Md5).toHex());
    }

    // 使用 PNG 格式，文件名包含哈希和尺寸
    QString fileName = QStringLiteral("%1_%2x%3.png").arg(hash).arg(size.width()).arg(size.height());
    return QDir(cacheDir).absoluteFilePath(fileName); // 使用 QDir 确保路径正确
}

} // namespace ThumbnailCacheUtils

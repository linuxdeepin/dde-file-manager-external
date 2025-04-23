// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef THUMBNAILCACHEUTILS_H
#define THUMBNAILCACHEUTILS_H

#include <QString>
#include <QSize>

namespace ThumbnailCacheUtils {

// 缓存目录的名称
const QString THUMBNAIL_CACHE_DIR_NAME = QStringLiteral("thumbnails");

/**
 * @brief 计算文件内容的 MD5 哈希值
 * @param filePath 文件路径
 * @return 文件的 MD5 哈希值 (十六进制字符串)，如果失败则返回空字符串
 */
QString calculateFileHash(const QString &filePath);

/**
 * @brief 获取缩略图的缓存文件路径
 * @param originalPath 原始文件的路径
 * @param size 缩略图的目标尺寸
 * @return 完整的缓存文件路径
 */
QString getThumbnailCachePath(const QString &originalPath, const QSize &size);

} // namespace ThumbnailCacheUtils

#endif // THUMBNAILCACHEUTILS_H

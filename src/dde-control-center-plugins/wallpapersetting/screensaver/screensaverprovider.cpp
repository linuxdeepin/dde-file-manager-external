// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screensaverprovider.h"

#include <QThread>
#include <QSet>
#include <QApplication>
#include <QScreen>
#include <QImageReader>
#include <QPainter>
#include "common/thumbnailcacheutils.h"

using namespace dfm_wallpapersetting;

ScreensaverWorker::ScreensaverWorker(ScreenSaverIfs *ifs, QObject *parent)
    : QObject(parent)
    , screensaverIfs(ifs)
{

}

void ScreensaverWorker::terminate()
{
    running = false;
}

void ScreensaverWorker::requestResource()
{
    running = true;
    screensaverIfs->allScreenSaver();
    QSet<QString> configurable = QSet<QString>::fromList(screensaverIfs->ConfigurableItems().value());
    QStringList saverNameList = screensaverIfs->allScreenSaver();

    // Supports parameter setting for multiple screensavers
    int deepin = 0;
    for (const QString &name : saverNameList) {
        // The screensaver with the parameter configuration is placed first
        if (name.startsWith("deepin")) {
            saverNameList.move(saverNameList.indexOf(name), deepin);
            deepin++;
        }
    }

    if (!running)
        return;

    QList<ItemNodePtr> items;
    QMap<QString, QString> imgs;

    for (const QString &name : saverNameList) {
        //romove
        if ("flurry" == name)
            continue;

        if (!running)
            return;

        auto temp = new ScreensaverItem;
        items.append(ItemNodePtr(temp));

        QString coverPath = screensaverIfs->GetScreenSaverCover(name);
        temp->item = name;
        temp->color = Qt::black;
        temp->configurable = configurable.contains(name);
        temp->deletable = false;
        temp->coverPath = coverPath;
        temp->selectable = true;

        imgs.insert(name, coverPath);
    }

    emit pushScreensaver(items);

    generateThumbnail(imgs);
    running = false;
}

void ScreensaverWorker::generateThumbnail(const QMap<QString, QString> &paths)
{
    const qreal ratio = qApp->primaryScreen()->devicePixelRatio();
    const int itemWidth = static_cast<int>(LISTVIEW_ICON_WIDTH * ratio);
    const int itemHeight = static_cast<int>(LISTVIEW_ICON_HEIGHT * ratio);
    const QSize size(itemWidth, itemHeight); // 目标缩略图尺寸

    for (auto it = paths.begin(); it != paths.end(); ++it) {
        if (!running) // 检查是否需要提前终止
            return;

        const QString screensaverName = it.key();
        const QString realPath = it.value(); // 原始封面图片路径

        if (realPath.isEmpty()) {
            qWarning() << "Skipping thumbnail generation for" << screensaverName << "due to empty path.";
            continue;
        }

        QString cachePath = ThumbnailCacheUtils::getThumbnailCachePath(realPath, size);
        QFileInfo cacheFile(cachePath);
        QPixmap pix;

        if (cacheFile.exists()) {
            // 尝试从缓存加载 - 使用 QImage 避免线程安全问题
            QImage cacheImage;
            if (cacheImage.load(cachePath)) {
                pix = QPixmap::fromImage(cacheImage);
                pix.setDevicePixelRatio(ratio); // 设置DPI
                emit pushThumbnail(screensaverName, pix);
                continue; // 缓存命中，处理下一个
            } else {
                qWarning() << "Failed to load screensaver thumbnail from cache:" << cachePath;
                // 缓存文件可能已损坏，尝试删除
                QFile::remove(cachePath);
            }
        }

        // --- 缓存未命中或加载失败，生成新的缩略图 ---

        QImage image(realPath);
        if (image.isNull()) {
            qWarning() << "Failed to load screensaver cover image:" << realPath;
            continue; // 加载失败，跳过此项
        }

        // 生成缩略图 - 在 QImage 层面进行操作避免线程安全问题
        QImage scaledImage = image.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        // 如果需要裁剪
        if (scaledImage.width() > itemWidth || scaledImage.height() > itemHeight) {
            const QRect sourceRect(scaledImage.rect().center() - QRect(0, 0, itemWidth, itemHeight).center(), size);
            scaledImage = scaledImage.copy(sourceRect);
        }

        if (!running)
            return;

        // 保存到缓存 - 使用 QImage 保存避免线程安全问题
        if (!scaledImage.save(cachePath, "PNG")) {
            qWarning() << "Failed to save screensaver thumbnail to cache:" << cachePath;
        }

        // 转换为 QPixmap 并发送信号
        pix = QPixmap::fromImage(scaledImage);

        // 设置DPI并发送信号
        pix.setDevicePixelRatio(ratio);
        emit pushThumbnail(screensaverName, pix);
    }
}

void ScreensaverProvider::setScreensaver(const QList<ItemNodePtr> &items)
{
    qDebug() << "get screensaver list" << items.size() << "current thread" << QThread::currentThread() << "main:" << qApp->thread();
    screensavers = items;
    screensaverMtx.unlock();
}

void ScreensaverProvider::setThumbnail(const QString &item, const QPixmap &pix)
{
    auto it = std::find_if(screensavers.begin(), screensavers.end(), [item](const ItemNodePtr &node){
        return node->item == item;
    });

    if (it == screensavers.end())
        return;
    (*it)->pixmap = pix;

    emit imageChanged(item);
}

ScreensaverProvider::ScreensaverProvider(QObject *parent) : QObject(parent)
{
    qDebug() << "create com.deepin.daemon.ScreenSaver.";
    screensaverIfs = new ScreenSaverIfs("com.deepin.ScreenSaver",
                                        "/com/deepin/ScreenSaver",
                                        QDBusConnection::sessionBus(), this);
    qDebug() << "end com.deepin.daemon.ScreenSaver.";

    workThread = new QThread(this);
    worker = new ScreensaverWorker(screensaverIfs);
    worker->moveToThread(workThread);
    workThread->start();

    connect(worker, &ScreensaverWorker::pushScreensaver, this, &ScreensaverProvider::setScreensaver, Qt::DirectConnection);
    connect(worker, &ScreensaverWorker::pushThumbnail, this, &ScreensaverProvider::setThumbnail, Qt::QueuedConnection);
}

ScreensaverProvider::~ScreensaverProvider()
{
    worker->terminate();
    workThread->quit();
    workThread->wait(1000);

    if (workThread->isRunning())
        workThread->terminate();

    delete worker;
    worker = nullptr;
}

void ScreensaverProvider::fecthData()
{
    // get picture
    if (screensaverMtx.tryLock(1)) {
        QMetaObject::invokeMethod(worker, "requestResource", Qt::QueuedConnection);
    } else {
        qWarning() << "screensaver is locked...";
    }
}

int ScreensaverProvider::getCurrentIdle()
{
    return screensaverIfs->linePowerScreenSaverTimeout();
}

void ScreensaverProvider::setCurrentIdle(int sec)
{
    screensaverIfs->setLinePowerScreenSaverTimeout(sec);
    screensaverIfs->setBatteryScreenSaverTimeout(sec);
}

bool ScreensaverProvider::getIsLock()
{
    return screensaverIfs->lockScreenAtAwake();
}

void ScreensaverProvider::setIsLock(bool l)
{
    screensaverIfs->setLockScreenAtAwake(l);
}

QString ScreensaverProvider::current()
{
    return screensaverIfs->currentScreenSaver();
}

void ScreensaverProvider::setCurrent(const QString &name)
{
    qInfo() << "set current screensaver" << name;
    if (name.isEmpty())
        return;

    screensaverIfs->setCurrentScreenSaver(name);
}

void ScreensaverProvider::configure(const QString &name)
{
    qInfo() << "configure screensaver" << name;
    if (name.isEmpty())
        return;

    screensaverIfs->StartCustomConfig(name);
}

void ScreensaverProvider::startPreview(const QString &name)
{
    qDebug() << "preview screensaver" << name;
    screensaverIfs->Preview(name, 1);
}

void ScreensaverProvider::stopPreview()
{
    qDebug() << "stop preview screensaver";
    screensaverIfs->Stop();
}

bool ScreensaverProvider::waitScreensaver(int ms) const
{
    bool ret = screensaverMtx.tryLock(ms);
    if (ret)
        screensaverMtx.unlock();
    return ret;
}

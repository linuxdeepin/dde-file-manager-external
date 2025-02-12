// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fillstylescombox.h"

#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>

using namespace dfm_wallpapersetting;
DCORE_USE_NAMESPACE

static constexpr char kConfName[] { "org.deepin.dde.file-manager.desktop" };
static constexpr char KwallpaperFillStyle[] { "wallpaperFillStyle" };

FillstylesCombox::FillstylesCombox(QFrame *parent)
    : ComboxWidget(parent)
{
    setTitle(tr("Fill Style"));
    addBackground();
    QStringList styles;
    styles << tr("Fill")   // Fill = 0
           << tr("Fit")   // Fit
           << tr("Stretch")   // Stretch
           << tr("Tile")   // Flatten (平铺)
           << tr("Center");   // Center

    comboBox()->addItems(styles);
    comboBox()->setCurrentIndex(0);
    fillStyleConf = DConfig::create("org.deepin.dde.file-manager", kConfName, "", this);
    connect(this, &ComboxWidget::onIndexChanged, this, &FillstylesCombox::setFillStyle);
}

FillstylesCombox::~FillstylesCombox()
{
}

void FillstylesCombox::reset(const QString &screen, int mode)
{
    setVisible(mode != 1);
    currentScreen = screen;

    if (!fillStyleConf) {
        qWarning() << "fillStyleConf is null";
        setVisible(false);
        return;
    }

    QString styleConfig = fillStyleConf->value(KwallpaperFillStyle).toString();
    int style = getValueFromJson(styleConfig, screen);
    qInfo() << "FillstylesCombox" << mode << "reset" << styleConfig << screen << style;
    comboBox()->setCurrentIndex(style);
}

void FillstylesCombox::setFillStyle(int index)
{
    if (!fillStyleConf) {
        qWarning() << "fillStyleConf is null";
        return;
    }

    QString jsonStr = fillStyleConf->value(KwallpaperFillStyle).toString();
    QString newJsonStr = updateJsonValue(jsonStr, currentScreen, index);
    qInfo() << "setFillStyle" << newJsonStr;
    fillStyleConf->setValue(KwallpaperFillStyle, newJsonStr);
}

int FillstylesCombox::getValueFromJson(QString json, const QString &screenName)
{
    //The string in dconfig contains extra characters
    if (json.startsWith('"')) {
        json.remove(0, 1);
    }
    if (json.endsWith('"')) {
        json.chop(1);
    }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json.toUtf8());

    if (!jsonDoc.isObject())
        return 0;

    QJsonObject jsonObj = jsonDoc.object();

    if (jsonObj.contains(screenName))
        return jsonObj[screenName].toInt();

    return 0;
}

QString FillstylesCombox::updateJsonValue(const QString &jsonStr, const QString &screenName, int value)
{
    QString json = jsonStr;

    QJsonDocument jsonDoc;
    QJsonObject jsonObj;

    if (!json.isEmpty()) {
        jsonDoc = QJsonDocument::fromJson(json.toUtf8());
        if (jsonDoc.isObject()) {
            jsonObj = jsonDoc.object();
        }
    }

    // Update or add the screen value
    jsonObj[screenName] = value;

    // Convert back to string
    jsonDoc.setObject(jsonObj);
    return QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact));
}

void FillstylesCombox::onFillStyleChanged(const QString &key)
{
    if (key != KwallpaperFillStyle)
        return;

    QString styleConfig = fillStyleConf->value(kConfName).toString();
    int style = getValueFromJson(styleConfig, currentScreen);
    comboBox()->setCurrentIndex(style);
}

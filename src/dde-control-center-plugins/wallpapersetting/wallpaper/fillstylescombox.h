// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILLSTYLESCOMBOX_H
#define FILLSTYLESCOMBOX_H

#include <widgets/comboxwidget.h>
#include <DConfig>

namespace dfm_wallpapersetting {

class FillstylesCombox : public dcc::widgets::ComboxWidget
{
    Q_OBJECT
public:
    explicit FillstylesCombox(QFrame *parent = nullptr);
    ~FillstylesCombox();

    void reset(const QString &screen, int mode);

public slots:
    void setFillStyle(int index);
    void onFillStyleChanged(const QString &key);

private:
    QString updateJsonValue(const QString &jsonStr, const QString &screenName, int value);
    int getValueFromJson(QString json, const QString &screenName);

private:
    DTK_CORE_NAMESPACE::DConfig *fillStyleConf = nullptr;
    QString currentScreen;
};

}

#endif   // FILLSTYLESCOMBOX_H

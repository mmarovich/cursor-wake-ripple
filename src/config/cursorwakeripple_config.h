/*
    SPDX-FileCopyrightText: 2026 Max Marovich
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KCModule>
#include <KPluginMetaData>

class QDoubleSpinBox;
class QSpinBox;

namespace KWin
{

class CursorWakeRippleEffectConfig : public KCModule
{
    Q_OBJECT
public:
    explicit CursorWakeRippleEffectConfig(QObject *parent, const KPluginMetaData &data);
    void load() override;
    void save() override;
    void defaults() override;

private:
    QSpinBox *m_idleSeconds = nullptr;
    QDoubleSpinBox *m_size = nullptr;
};

} // namespace KWin

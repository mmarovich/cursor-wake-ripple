/*
    SPDX-FileCopyrightText: 2026 Max Marovich
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "cursorwakeripple_config.h"

#include <KConfigGroup>

#include <algorithm>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

K_PLUGIN_CLASS_WITH_JSON(KWin::CursorWakeRippleEffectConfig, "cursorwakeripple_config.json")

namespace KWin
{

CursorWakeRippleEffectConfig::CursorWakeRippleEffectConfig(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    auto *w = widget();
    auto *layout = new QVBoxLayout(w);
    auto *form = new QFormLayout;
    layout->addLayout(form);
    layout->addStretch(1);

    m_idleSeconds = new QSpinBox(w);
    m_idleSeconds->setRange(1, 600);
    m_idleSeconds->setSuffix(i18n(" seconds"));
    m_idleSeconds->setToolTip(i18n("How long the mouse must sit still before the next move triggers a ripple."));
    form->addRow(i18n("Idle time before trigger:"), m_idleSeconds);

    m_size = new QDoubleSpinBox(w);
    m_size->setRange(0.05, 0.60);
    m_size->setSingleStep(0.01);
    m_size->setDecimals(2);
    m_size->setToolTip(i18n("How far the ripple expands, as a fraction of screen height."));
    form->addRow(i18n("Ripple size:"), m_size);

    auto *hint = new QLabel(i18n("Tip: start around 30 seconds of idle time once you’re done testing."), w);
    hint->setWordWrap(true);
    layout->insertWidget(1, hint);

    connect(m_idleSeconds, qOverload<int>(&QSpinBox::valueChanged), this, &KCModule::markAsChanged);
    connect(m_size, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &KCModule::markAsChanged);
}

void CursorWakeRippleEffectConfig::load()
{
    const KConfigGroup cg = KSharedConfig::openConfig(QStringLiteral("kwinrc"))->group(QStringLiteral("Effect-cursorwakeripple"));
    m_idleSeconds->setValue(std::max(1, cg.readEntry("IdleTimeoutMs", 1000) / 1000));
    m_size->setValue(cg.readEntry("MaxRadius", 0.18));
    setNeedsSave(false);
}

void CursorWakeRippleEffectConfig::defaults()
{
    m_idleSeconds->setValue(30); // practical default for the Defaults button
    m_size->setValue(0.18);
    markAsChanged();
}

void CursorWakeRippleEffectConfig::save()
{
    KConfigGroup cg = KSharedConfig::openConfig(QStringLiteral("kwinrc"))->group(QStringLiteral("Effect-cursorwakeripple"));
    cg.writeEntry("IdleTimeoutMs", m_idleSeconds->value() * 1000);
    cg.writeEntry("MaxRadius", m_size->value());
    cg.sync();

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                      QStringLiteral("/Effects"),
                                                      QStringLiteral("org.kde.kwin.Effects"),
                                                      QStringLiteral("reconfigureEffect"));
    msg.setArguments({QStringLiteral("cursorwakeripple")});
    QDBusConnection::sessionBus().call(msg);

    setNeedsSave(false);
}

} // namespace KWin

#include "cursorwakeripple_config.moc"

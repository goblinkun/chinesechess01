// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2019-2026 XMuli & Contributors
// SPDX-GitHub: https://github.com/XMuli/ChineseChess
// SPDX-Author: XMuli <xmulitech@gmail.com>

#include "AboutAuthor.h"
#include "ui_AboutAuthor.h"
#include <QIcon>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>

AboutAuthor::AboutAuthor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutAuthor)
{
    this->setWindowIcon(QIcon(":/images/chess.svg"));
    ui->setupUi(this);

    // 本版本为皮卡鱼引擎改编版：移除原作者贡献者墙，改为改编声明
    ui->gridWidget->setVisible(false);

    auto *credit = new QLabel(this);
    credit->setTextFormat(Qt::RichText);
    credit->setOpenExternalLinks(true);
    credit->setAlignment(Qt::AlignCenter);
    credit->setStyleSheet(QStringLiteral("color:#1f1f1f; font-size:14pt;"));
    credit->setText(QStringLiteral(
        "<p style='margin:20px 0;'>老地精引用皮卡鱼引擎改编</p>"
        "<p style='font-size:11pt;'>引擎源码："
        "<a href='https://github.com/official-pikafish/Pikafish' "
        "style='color:#007af4;'>https://github.com/official-pikafish/Pikafish</a></p>"));

    auto *lay = qobject_cast<QVBoxLayout*>(layout());
    if (lay) {
        const int idx = lay->indexOf(ui->gridWidget);
        if (idx >= 0)
            lay->insertWidget(idx, credit);
    }
}

AboutAuthor::~AboutAuthor()
{
    delete ui;
}

// void AboutAuthor::paintEvent(QPaintEvent *event)
// {
//     QPainter painter(this);
//     painter.drawPixmap(rect(), QPixmap(":/images/background.jpg"));

//     QDialog::paintEvent(event);
// }

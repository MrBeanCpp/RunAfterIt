#include "item.h"
#include "ui_item.h"
#include <QDebug>
Item::Item(QListWidgetItem* _widgetItem, QWidget* parent)
    : QWidget(parent), widgetItem(_widgetItem), ui(new Ui::Item)
{
    ui->setupUi(this);

    connect(ui->check_used, &QCheckBox::toggled, this, &Item::contentChanged);
    connect(ui->edit_target, &LineEdit::pathChanged, this, [=](QString path){
        bool isWifi = ItemInfo::isWifi(path);
        ui->btn_endWith->setEnabled(!isWifi); //wifi时禁用 可以支持但没必要
        emit contentChanged();
    });
    connect(ui->edit_follow, &LineEdit::pathChanged, this, &Item::contentChanged);
    connect(ui->btn_endWith, &QToolButton::toggled, this, &Item::contentChanged);
    connect(ui->btn_loop, &QToolButton::toggled, this, &Item::contentChanged);

    connect(ui->check_used, &QCheckBox::toggled, this, [=](bool checked) {
        ui->edit_target->setEnabled(checked);
        ui->edit_follow->setEnabled(checked);
        ui->btn_endWith->setEnabled(checked);
        ui->btn_loop->setEnabled(checked);
    });

    connect(ui->btn_del, &QToolButton::clicked, this, [=]() {
        emit deleteButtonClicked(widgetItem);
    });
}

Item::~Item()
{
    delete ui;
}

void Item::setData(const ItemInfo& info)
{
    ui->check_used->setChecked(info.isUsed);
    ui->edit_target->setPath(info.target);
    ui->edit_follow->setPath(info.follow);
    ui->btn_endWith->setChecked(info.isEndWith);
    ui->btn_loop->setChecked(info.isLoop);
}

ItemInfo Item::getData()
{
    return ItemInfo(
        ui->check_used->isChecked(),
        ui->edit_target->getPath(),
        ui->edit_follow->getPath(),
        ui->btn_endWith->isChecked(),
        ui->btn_loop->isChecked());
}

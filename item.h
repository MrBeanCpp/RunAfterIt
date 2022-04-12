#ifndef ITEM_H
#define ITEM_H

#include "iteminfo.h"
#include <QString>
#include <QWidget>
#include <QListWidgetItem>
namespace Ui {
class Item;
}

class Item : public QWidget
{
    Q_OBJECT

public:
    explicit Item(QListWidgetItem* _widgetItem, QWidget* parent = nullptr);
    ~Item();

    void setData(const ItemInfo& info);
    ItemInfo getData(void);

signals:
    void contentChanged(void);
    void deleteButtonClicked(QListWidgetItem* widgetItem);

private:
    QListWidgetItem* widgetItem;
    Ui::Item* ui;
};

#endif // ITEM_H

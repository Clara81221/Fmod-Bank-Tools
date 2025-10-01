#ifndef FMOD_FSB_LIST_H
#define FMOD_FSB_LIST_H

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class Fmod_FSB_List;
}

class Fmod_FSB_List : public QDialog
{
    Q_OBJECT

public:
    explicit Fmod_FSB_List(QWidget *parent = nullptr);
    ~Fmod_FSB_List();

private:
    Ui::Fmod_FSB_List *ui;
};

#endif // FMOD_FSB_LIST_H

#ifndef VIRTUAL_PROPERTIES_H
#define VIRTUAL_PROPERTIES_H

#include <QDialog>

namespace Ui {
class VirtualProperties;
}

class VirtualProperties : public QDialog
{
    Q_OBJECT

public:
    explicit VirtualProperties(QWidget *parent = 0);
    ~VirtualProperties();
	void SetVisable();
	void showEvent(QShowEvent *event);
	bool output_enable=false;

private Q_SLOTS:
	void onStart();
	void onStop();

private:
    Ui::VirtualProperties *ui;
};

#endif // VIRTUAL_PROPERTIES_H

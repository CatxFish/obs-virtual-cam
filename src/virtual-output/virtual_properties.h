#ifndef VIRTUAL_PROPERTIES_H
#define VIRTUAL_PROPERTIES_H

#include <QDialog>
#include "obs.h"

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
	void closeEvent(QCloseEvent *event);
	bool output_enable=false;

private Q_SLOTS:
	void onStart();
	void onStop();
	void onGetSourceRegion();
	void onChangeCropValue();

private:
    Ui::VirtualProperties *ui;
	struct obs_video_info video_info;
	QString scene_name;
	void ValidateRegion(int& left, int& top, int& right, int& bottom);
	bool GetItemRegion(obs_sceneitem_t* item,int& left, int& top, int& right, 
		int& bottom);
	void SaveSetting();
	static bool ListSource(obs_scene_t* scene, obs_sceneitem_t* item, 
		void* ui);
};

#endif // VIRTUAL_PROPERTIES_H

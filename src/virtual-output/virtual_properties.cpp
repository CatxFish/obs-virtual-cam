#include "virtual_properties.h"
#include "ui_virtual_properties.h"
#include "virtual_output.h"
#include <math.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>
#include <qmessagebox.h>

VirtualProperties::VirtualProperties(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VirtualProperties)
{
    ui->setupUi(this);
	connect(ui->pushButtonStart, SIGNAL(clicked()),
		this, SLOT(onStart()));
	connect(ui->pushButtonStop, SIGNAL(clicked()), this, SLOT(onStop()));
	connect(ui->spinBox, SIGNAL(valueChanged(int)), 
		ui->horizontalSlider, SLOT(setValue(int)));
	connect(ui->horizontalSlider, SIGNAL(valueChanged(int)), ui->spinBox,
		SLOT(setValue(int)));
	connect(ui->checkBox_horiflip, SIGNAL(stateChanged(int)), this,
		SLOT(onClickHorizontalFlip()));
	connect(ui->checkBox_keepratio, SIGNAL(stateChanged(int)), this,
		SLOT(onClickKeepAspectRatio()));

	config_t* config = obs_frontend_get_global_config();
	config_set_default_bool(config, "VirtualOutput", "AutoStart", false);
	config_set_default_bool(config, "VirtualOutput", "HoriFlip", false);
	config_set_default_bool(config, "VirtualOutput", "KeepRatio", false);
	config_set_default_int(config, "VirtualOutput", "OutDelay", 3);	
	bool autostart = config_get_bool(config, "VirtualOutput", "AutoStart");
	bool hori_flip = config_get_bool(config, "VirtualOutput", "HoriFlip");
	bool keep_ratio = config_get_bool(config, "VirtualOutput", "KeepRatio");
	int delay = config_get_int(config, "VirtualOutput", "OutDelay");

	
	ui->checkBox_auto->setChecked(autostart);
	ui->checkBox_horiflip->setChecked(hori_flip);
	ui->checkBox_keepratio->setChecked(keep_ratio);
	ui->spinBox->setValue(delay);
	ui->horizontalSlider->setValue(delay);
	update_data.keep_ratio = keep_ratio;
	update_data.horizontal_flip = hori_flip;

	if (autostart) {
		obs_get_video_info(&video_info);
		onStart();
	}
}

VirtualProperties::~VirtualProperties()
{
	virtual_output_disable();
	SaveSetting();
    delete ui;
}

void VirtualProperties::SetVisable()
{
	setVisible(!isVisible());
}

void VirtualProperties::onStart()
{
	int delay = ui->horizontalSlider->value();
	ui->spinBox->setEnabled(false);
	ui->horizontalSlider->setEnabled(false);
	virtual_output_enable(delay + 1);
	ui->pushButtonStart->setEnabled(false);
	ui->pushButtonStop->setEnabled(true);
	UpdateParameter();
}

void VirtualProperties::onStop()
{
	virtual_output_disable();
	ui->spinBox->setEnabled(true);
	ui->horizontalSlider->setEnabled(true);
	ui->pushButtonStart->setEnabled(true);
	ui->pushButtonStop->setEnabled(false);
}

void VirtualProperties::UpdateParameter()
{

	update_data.horizontal_flip = ui->checkBox_horiflip->isChecked();
	update_data.keep_ratio = ui->checkBox_keepratio->isChecked();
	virtual_output_set_data(&update_data);
}

void VirtualProperties::onClickHorizontalFlip()
{
	UpdateParameter();
}

void VirtualProperties::onClickKeepAspectRatio()
{
	UpdateParameter();
}

void VirtualProperties::showEvent(QShowEvent *event)
{
	if (virtual_output_occupied()) {
		ui->spinBox->setEnabled(false);
		ui->horizontalSlider->setEnabled(false);
		ui->pushButtonStart->setEnabled(false);
		ui->pushButtonStop->setEnabled(false);
	} else {
		ui->spinBox->setEnabled(!virtual_output_is_running());
		ui->horizontalSlider->setEnabled(!virtual_output_is_running());
		ui->pushButtonStart->setEnabled(!virtual_output_is_running());
		ui->pushButtonStop->setEnabled(virtual_output_is_running());
	}
}

void VirtualProperties::closeEvent(QCloseEvent *event)
{
	SaveSetting();
}

void VirtualProperties::SaveSetting()
{
	config_t* config = obs_frontend_get_global_config();
	if (config) {
		bool autostart = ui->checkBox_auto->isChecked();
		bool hori_flip = ui->checkBox_horiflip->isChecked();
		bool keep_ratio = ui->checkBox_keepratio->isChecked();
		int delay = ui->horizontalSlider->value();
		config_set_bool(config, "VirtualOutput", "AutoStart", autostart);
		config_set_bool(config, "VirtualOutput", "HoriFlip", hori_flip);
		config_set_bool(config, "VirtualOutput", "KeepRatio", keep_ratio);
		config_set_int(config, "VirtualOutput", "OutDelay", delay);
	}
}
#include "virtual_properties.h"
#include "ui_virtual_properties.h"
#include "virtual_output.h"
#include <math.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

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
	config_set_default_int(config, "VirtualOutput", "Target", 0);
	bool autostart = config_get_bool(config, "VirtualOutput", "AutoStart");
	bool hori_flip = config_get_bool(config, "VirtualOutput", "HoriFlip");
	bool keep_ratio = config_get_bool(config, "VirtualOutput", "KeepRatio");
	int delay = config_get_int(config, "VirtualOutput", "OutDelay");
	int target = config_get_int(config, "VirtualOutput", "Target");

	
	ui->checkBox_auto->setChecked(autostart);
	ui->checkBox_horiflip->setChecked(hori_flip);
	ui->checkBox_keepratio->setChecked(keep_ratio);
	ui->comboBox_target->addItem("OBS-Camera", ModeVideo);
	ui->comboBox_target->addItem("OBS-Camera2", ModeVideo2);
	ui->comboBox_target->addItem("OBS-Camera3", ModeVideo3);
	ui->comboBox_target->addItem("OBS-Camera4", ModeVideo4);
	ui->comboBox_target->setCurrentIndex(
		ui->comboBox_target->findData(target));
	ui->spinBox->setValue(delay);
	ui->horizontalSlider->setValue(delay);
	ui->label->setStyleSheet("QLabel { color : red; }");
	ui->label->setVisible(false);
	ui->pushButtonStop->setEnabled(false);
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
	virtual_output_terminate();
	SaveSetting();
    delete ui;
}

void VirtualProperties::SetVisable()
{
	setVisible(!isVisible());
}

void VirtualProperties::EnableOptions(bool enable)
{
	ui->spinBox->setEnabled(enable);
	ui->horizontalSlider->setEnabled(enable);
	ui->comboBox_target->setEnabled(enable);
	ui->pushButtonStart->setEnabled(enable);
	ui->pushButtonStop->setEnabled(!enable);
}

void VirtualProperties::ShowWarning(bool show)
{
	ui->label->setVisible(show);
}

void VirtualProperties::onStart()
{
	ShowWarning(false);
	UpdateParameter();
	EnableOptions(false);
	virtual_signal_connect("output_stop", onStopSignal, this);
	virtual_output_enable();
}

void VirtualProperties::onStop()
{
	virtual_output_disable();
}

void VirtualProperties::onStopSignal(void *data, calldata_t *cd)
{
	auto page = (VirtualProperties*)data;
	bool start_fail = calldata_bool(cd, "start_fail");
	if (start_fail)
		page->ShowWarning(true);
	page->EnableOptions(true);
	virtual_signal_disconnect("output_stop", onStopSignal, data);
}

void VirtualProperties::UpdateParameter()
{

	update_data.horizontal_flip = ui->checkBox_horiflip->isChecked();
	update_data.keep_ratio = ui->checkBox_keepratio->isChecked();
	update_data.delay = ui->horizontalSlider->value();
	update_data.mode = ui->comboBox_target->currentData().toInt();
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
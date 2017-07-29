#include "virtual_properties.h"
#include "ui_virtual_properties.h"
#include "virtual_output.h"
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

}

VirtualProperties::~VirtualProperties()
{
    delete ui;
}

void VirtualProperties::SetVisable()
{
	setVisible(!isVisible());
}

void VirtualProperties::onStart()
{
	config_t* config = obs_frontend_get_global_config();
	output_enable = true;
	int delay = ui->horizontalSlider->value();

	if (config){
		config_set_bool(config, "VirtualOutput", "OutEnable", output_enable);
		config_set_int(config, "VirtualOutput", "OutDelay", delay+1);
	}

	ui->spinBox->setEnabled(false);
	ui->horizontalSlider->setEnabled(false);
	virtual_output_enable(delay);
	ui->pushButtonStart->setEnabled(false);
	ui->pushButtonStop->setEnabled(true);
}

void VirtualProperties::onStop()
{
	config_t* config = obs_frontend_get_global_config();
	output_enable = true;

	if (config)
		config_set_bool(config, "VirtualOutput", "OutEnable", output_enable);

	virtual_output_disable();

	ui->spinBox->setEnabled(true);
	ui->horizontalSlider->setEnabled(true);
	ui->pushButtonStart->setEnabled(true);
	ui->pushButtonStop->setEnabled(false);
}

void VirtualProperties::showEvent(QShowEvent *event)
{
	config_t* config = obs_frontend_get_global_config();
	output_enable = config_get_bool(config, "VirtualOutput", "OutEnable");
	int delay = config_get_int(config, "VirtualOutput", "OutDelay");

	if (delay < 0 || delay >30)
		delay = 0;

	ui->spinBox->setValue(delay);
	ui->horizontalSlider->setValue(delay);
	if (virtual_output_occupied()){
		ui->spinBox->setEnabled(false);
		ui->horizontalSlider->setEnabled(false);
		ui->pushButtonStart->setEnabled(false);
		ui->pushButtonStop->setEnabled(false);
	}
	else{
		ui->spinBox->setEnabled(!virtual_output_is_running());
		ui->horizontalSlider->setEnabled(!virtual_output_is_running());
		ui->pushButtonStart->setEnabled(!virtual_output_is_running());
		ui->pushButtonStop->setEnabled(virtual_output_is_running());
	}
}

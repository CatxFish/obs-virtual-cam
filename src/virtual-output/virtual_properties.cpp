#include "virtual_properties.h"
#include "ui_virtual_properties.h"
#include "virtual_output.h"
#include <math.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>
#include <qmessagebox.h>

#define REGION_MIN_WIDTH 32
#define REGION_MIN_HEIGHT 32

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
	connect(ui->pushButton_get_region, SIGNAL(clicked()), this,
		SLOT(onGetSourceRegion()));
	connect(ui->spinBox_left, SIGNAL(valueChanged(int)), this, 
		SLOT(onChangeCropValue()));
	connect(ui->spinBox_top, SIGNAL(valueChanged(int)), this, 
		SLOT(onChangeCropValue()));
	connect(ui->spinBox_right, SIGNAL(valueChanged(int)), this, 
		SLOT(onChangeCropValue()));
	connect(ui->spinBox_bottom, SIGNAL(valueChanged(int)), this, 
		SLOT(onChangeCropValue()));
	connect(ui->radioButton_enable_crop, SIGNAL(toggled(bool)), this,
		SLOT(onChangeCropEnable()));
	connect(ui->radioButton_disable_crop, SIGNAL(toggled(bool)), this, 
		SLOT(onChangeCropEnable()));
	connect(ui->checkBox_horiflip, SIGNAL(stateChanged(int)), this,
		SLOT(onClickHorizontalFlip()));
	connect(ui->checkBox_keepratio, SIGNAL(stateChanged(int)), this,
		SLOT(onClickKeepAspectRatio()));

	config_t* config = obs_frontend_get_global_config();
	config_set_default_bool(config, "VirtualOutput", "AutoStart", false);
	config_set_default_bool(config, "VirtualOutput", "HoriFlip", false);
	config_set_default_bool(config, "VirtualOutput", "KeepRatio", false);
	config_set_default_bool(config, "VirtualOutput", "CropEnable", false);
	config_set_default_int(config, "VirtualOutput", "OutDelay", 3);	
	config_set_default_int(config, "VirtualOutput", "CropLeft", 0);
	config_set_default_int(config, "VirtualOutput", "CropRight", 0);
	config_set_default_int(config, "VirtualOutput", "CropTop", 0);
	config_set_default_int(config, "VirtualOutput", "CropBottom", 0);
	bool autostart = config_get_bool(config, "VirtualOutput", "AutoStart");
	bool hori_flip = config_get_bool(config, "VirtualOutput", "HoriFlip");
	bool keep_ratio = config_get_bool(config, "VirtualOutput", "KeepRatio");
	bool crop_enable = config_get_bool(config, "VirtualOutput", "CropEnable");
	int delay = config_get_int(config, "VirtualOutput", "OutDelay");
	int crop_left = config_get_int(config, "VirtualOutput", "CropLeft");
	int crop_right = config_get_int(config, "VirtualOutput", "CropRight");
	int crop_top = config_get_int(config, "VirtualOutput", "CropTop");
	int crop_bottom= config_get_int(config, "VirtualOutput", "CropBottom");
	
	ui->checkBox_auto->setChecked(autostart);
	ui->checkBox_horiflip->setChecked(hori_flip);
	ui->checkBox_keepratio->setChecked(keep_ratio);
	ui->radioButton_enable_crop->setChecked(crop_enable);
	ui->radioButton_disable_crop->setChecked(!crop_enable);
	ui->spinBox->setValue(delay);
	ui->horizontalSlider->setValue(delay);
	ui->spinBox_left->setValue(crop_left);
	ui->spinBox_right->setValue(crop_right);
	ui->spinBox_top->setValue(crop_top);
	ui->spinBox_bottom->setValue(crop_bottom);

	update_data.keep_ratio = keep_ratio;
	update_data.horizontal_flip = hori_flip;
	if (crop_enable) {
		update_data.crop[0] = crop_left;
		update_data.crop[1] = crop_top;
		update_data.crop[2] = crop_right;
		update_data.crop[3] = crop_bottom;
	}

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
	if (ui->radioButton_enable_crop->isChecked()){
		update_data.crop[0] = ui->spinBox_left->value();
		update_data.crop[1] = ui->spinBox_top->value();
		update_data.crop[2] = ui->spinBox_right->value();
		update_data.crop[3] = ui->spinBox_bottom->value();
		ValidateRegion(update_data.crop[0], update_data.crop[1],
			update_data.crop[2], update_data.crop[3]);		
	}
	else{
		update_data.crop[0] = 0;
		update_data.crop[1] = 0;
		update_data.crop[2] = 0;
		update_data.crop[3] = 0;
	}
	update_data.horizontal_flip = ui->checkBox_horiflip->isChecked();
	update_data.keep_ratio = ui->checkBox_keepratio->isChecked();
	virtual_output_set_data(&update_data);
}

void VirtualProperties::onChangeCropValue()
{
	if (ui->radioButton_enable_crop->isChecked())
		UpdateParameter();
}

void VirtualProperties::onChangeCropEnable()
{
	bool enable = ui->radioButton_enable_crop->isChecked();
	ui->listWidget_source->setEnabled(enable);
	ui->pushButton_get_region->setEnabled(enable);
	ui->spinBox_bottom->setEnabled(enable);
	ui->spinBox_top->setEnabled(enable);
	ui->spinBox_left->setEnabled(enable);
	ui->spinBox_right->setEnabled(enable);
	UpdateParameter();
}

void VirtualProperties::onClickHorizontalFlip()
{
	UpdateParameter();
}

void VirtualProperties::onClickKeepAspectRatio()
{
	UpdateParameter();
}

bool VirtualProperties::GetItemRegion(obs_sceneitem_t* item,
	int& left, int& top, int& right, int& bottom)
{
	float width, height;
	float offset_left,offset_top,offset_right,offset_bottom;
	float max_width = video_info.base_width;
	float max_height = video_info.base_height;
	obs_transform_info info;
	obs_sceneitem_get_info(item, &info);
	
	if (info.bounds_type == OBS_BOUNDS_NONE){
		obs_source_t* item_source = obs_sceneitem_get_source(item);
		int source_base_width = obs_source_get_base_width(item_source);
		int source_base_height = obs_source_get_height(item_source);
		width = round(source_base_width *info.scale.x);
		height = round(source_base_height*info.scale.y);
	}
	else{
		width = info.bounds.x;
		height = info.bounds.y;
	}

	float weight_sin = sin(info.rot*M_PI / 180.0);
	float weight_cos = cos(info.rot*M_PI / 180.0);

	if (info.rot <= 90.0){
		offset_left = -height * weight_sin;
		offset_top = 0;	
		offset_right = width * weight_cos;
		offset_bottom = width * weight_sin + height* weight_cos;
	}
	else if (info.rot <=180.0 && info.rot>90.0){
		offset_left = width * weight_cos - height * weight_sin;
		offset_top = height * weight_cos;
		offset_right = 0;
		offset_bottom = width*weight_sin;
	}
	else if (info.rot <=270.0 && info.rot>180.0){
		offset_left = width * weight_cos;
		offset_top = width * weight_sin + height *weight_cos;
		offset_right = -height * weight_sin;
		offset_bottom = 0;
	}
	else if (info.rot>270.0){
		offset_left = 0;
		offset_top = width* weight_sin;
		offset_right = width * weight_cos - height * weight_sin;
		offset_bottom = height * weight_cos;
	}


	offset_left = fmin(max_width, fmax(0, info.pos.x + offset_left));
	offset_top = fmin(max_height, fmax(0, info.pos.y + offset_top));
	offset_right = fmin(max_width, fmax(0, info.pos.x + offset_right));
	offset_bottom = fmin(max_height, fmax(0, info.pos.y + offset_bottom));

	if ((offset_top ==0 && offset_bottom ==0)||
	    (offset_top == max_height && offset_bottom == max_height)||
	    (offset_left == 0 && offset_right == 0) ||
	    (offset_left == max_width && offset_right == max_width)){ 

		left = 0;
		right = 0;
		top = 0;
		bottom = 0;
		return false;	
	} else{
		left = offset_left;
		top = offset_top;
		right = max_width - offset_right;
		bottom = max_height - offset_bottom;
	}

	return true;
}

void VirtualProperties::onGetSourceRegion()
{
	obs_source_t* source = obs_get_source_by_name(
		scene_name.toUtf8().constData());
	obs_scene_t* scene = obs_scene_from_source(source);
	QListWidgetItem * list_item = ui->listWidget_source->currentItem();
	if (scene && list_item) {
		QString name = list_item->text();
		obs_sceneitem_t* item = obs_scene_find_source(scene, 
			name.toUtf8().constData());
		if (item) {
			int left, top, right, bottom;
			if (GetItemRegion(item, left, top, right, bottom)) {
				ui->spinBox_left->setValue(left);
				ui->spinBox_top->setValue(top);
				ui->spinBox_right->setValue(right);
				ui->spinBox_bottom->setValue(bottom);
			}
		}
		obs_source_release(source);
	}
}

void VirtualProperties::showEvent(QShowEvent *event)
{
	obs_get_video_info(&video_info);
	obs_source_t* source = obs_frontend_get_current_scene();
	obs_scene_t* scene = obs_scene_from_source(source);
	if (scene) {
		scene_name = QString::fromUtf8(obs_source_get_name(source));
		ui->listWidget_source->clear();
		obs_scene_enum_items(scene, ListSource, (void*)ui->listWidget_source);
	}
	obs_source_release(source);

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

void VirtualProperties::ValidateRegion(int& left, int& top, int& right, 
	int& bottom)
{

	int crop_max_width = video_info.base_width - REGION_MIN_WIDTH;
	int crop_max_height = video_info.base_height - REGION_MIN_HEIGHT;

	if (left > crop_max_width)
		left = crop_max_width;

	if (right > crop_max_width - left)
		right = crop_max_width - left;

	if (top > crop_max_height)
		top = crop_max_height;

	if (bottom > crop_max_height - top)
		bottom = crop_max_height - top;

}

bool VirtualProperties::ListSource(obs_scene_t* scene, 
	obs_sceneitem_t* item, void* ui)
{
	QListWidget* cl = (QListWidget*)ui;
	obs_source_t* source = obs_sceneitem_get_source(item);
	const char* name = obs_source_get_name(source);
	cl->addItem(name);
	return true;
}

void VirtualProperties::SaveSetting()
{
	config_t* config = obs_frontend_get_global_config();
	if (config) {
		bool autostart = ui->checkBox_auto->isChecked();
		bool hori_flip = ui->checkBox_horiflip->isChecked();
		bool keep_ratio = ui->checkBox_keepratio->isChecked();
		bool crop_enable = ui->radioButton_enable_crop->isChecked();
		int delay = ui->horizontalSlider->value();
		int crop_left = ui->spinBox_left->value();
		int crop_right = ui->spinBox_right->value();
		int crop_top = ui->spinBox_top->value();
		int crop_bottom = ui->spinBox_bottom->value();
		config_set_bool(config, "VirtualOutput", "AutoStart", autostart);
		config_set_bool(config, "VirtualOutput", "HoriFlip", hori_flip);
		config_set_bool(config, "VirtualOutput", "KeepRatio", keep_ratio);
		config_set_bool(config, "VirtualOutput", "CropEnable", crop_enable);
		config_set_int(config, "VirtualOutput", "OutDelay", delay);
		config_set_int(config, "VirtualOutput", "CropLeft", crop_left);
		config_set_int(config, "VirtualOutput", "CropRight", crop_right);
		config_set_int(config, "VirtualOutput", "CropTop", crop_top);
		config_set_int(config, "VirtualOutput", "CropBottom", crop_bottom);	
	}
}
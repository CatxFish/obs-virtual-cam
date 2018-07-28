#include <obs-module.h>
#include <QMainWindow>
#include <QAction>
#include <obs-frontend-api.h>
#include "virtual_properties.h"

extern struct obs_output_info create_output_info();
struct obs_output_info ndi_output_info;

extern struct obs_source_info create_filter_info();
struct obs_source_info ndi_filter_info;

VirtualProperties* virtual_prop;


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-virtual_output", "en-US")

bool obs_module_load(void)
{
	obs_output_info virtual_output_info = create_output_info();
	obs_register_output(&virtual_output_info);

	obs_source_info virtual_filter_info = create_filter_info();
	obs_register_source(&virtual_filter_info);

	QMainWindow* main_window = (QMainWindow*)obs_frontend_get_main_window();
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(
		obs_module_text("VirtualCam"));

	obs_frontend_push_ui_translation(obs_module_get_string);
	virtual_prop = new VirtualProperties(main_window);
	obs_frontend_pop_ui_translation();

	auto menu_cb = []
	{
		virtual_prop->setVisible(!virtual_prop->isVisible());
	};

	action->connect(action, &QAction::triggered, menu_cb);

	return true;
}

void obs_module_unload(void)
{
}
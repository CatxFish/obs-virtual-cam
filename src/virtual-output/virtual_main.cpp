#include <obs-module.h>
#include <QMainWindow>
#include <QAction>
#include <obs-frontend-api.h>
#include "virtual_output.h"
#include "virtual_filter.h"
#include "virtual_properties.h"

VirtualProperties* virtual_prop;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-virtual_output", "en-US")

bool obs_module_load(void)
{
	virtual_output_init();
	virtual_filter_init();

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
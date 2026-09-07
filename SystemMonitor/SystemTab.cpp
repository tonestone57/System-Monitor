#include "SystemTab.h"
#include "SystemDetailsView.h"

#include <LayoutBuilder.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemTab"

SystemTab::SystemTab()
	: BView("SystemTab", B_WILL_DRAW | B_SUPPORTS_LAYOUT)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.Add(new SystemDetailsView())
		.End();
}

SystemTab::~SystemTab()
{
}

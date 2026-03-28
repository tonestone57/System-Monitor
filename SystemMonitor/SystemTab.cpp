#include "SystemTab.h"
#include "SystemSummaryView.h"
#include "SystemDetailsView.h"

#include <TabView.h>
#include <LayoutBuilder.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemTab"

SystemTab::SystemTab()
	: BView("SystemTab", B_WILL_DRAW)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.Add(new SystemSummaryView(), 1.0f)
		.Add(new SystemDetailsView(), 1.0f)
		.End();
}

SystemTab::~SystemTab()
{
}

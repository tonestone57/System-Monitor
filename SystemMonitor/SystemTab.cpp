#include "SystemTab.h"
#include "SystemSummaryView.h"
#include "SystemDetailsView.h"

#include <LayoutBuilder.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemTab"

SystemTab::SystemTab()
	: BView("SystemTab", B_WILL_DRAW)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.Add(new SystemSummaryView())
		.Add(new SystemDetailsView())
		.End();
}

SystemTab::~SystemTab()
{
}

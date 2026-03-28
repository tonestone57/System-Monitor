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

	BTabView* tabView = new BTabView("system_tab_view");

	tabView->AddTab(new SystemSummaryView());
	tabView->TabAt(0)->SetLabel(B_TRANSLATE("Summary"));

	tabView->AddTab(new SystemDetailsView());
	tabView->TabAt(1)->SetLabel(B_TRANSLATE("Details"));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(tabView)
		.End();
}

SystemTab::~SystemTab()
{
}

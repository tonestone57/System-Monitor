#include "SystemTab.h"
#include "SystemSummaryView.h"

#include <LayoutBuilder.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SystemTab"

SystemTab::SystemTab()
	: BView("SystemTab", B_WILL_DRAW)
{
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(new SystemSummaryView())
		.End();
}

SystemTab::~SystemTab()
{
}

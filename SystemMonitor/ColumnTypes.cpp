#include "ColumnTypes.h"
#include <View.h>
#include <stdio.h>

// --- SysMonStringField ---

SysMonStringField::SysMonStringField(const char* string)
    : fString(string)
{
}

SysMonStringField::~SysMonStringField()
{
}

void SysMonStringField::SetString(const char* string)
{
    fString = string;
}

const char* SysMonStringField::String() const
{
    return fString.String();
}

void SysMonStringField::SetClippedString(const char* string)
{
    fClippedString = string;
}

const char* SysMonStringField::ClippedString()
{
    return fClippedString.String();
}


// --- SysMonStringColumn ---

SysMonStringColumn::SysMonStringColumn(const char* title, float width, float minWidth,
                  float maxWidth, uint32 truncate, alignment align)
    : BColumn(title, width, minWidth, maxWidth, align),
      fTruncate(truncate)
{
}

void SysMonStringColumn::DrawField(BField* field, BRect rect, BView* parent)
{
    SysMonStringField* stringField = static_cast<SysMonStringField*>(field);
    if (stringField) {
        BString str = stringField->String();

        font_height fh;
        parent->GetFontHeight(&fh);
        float y = rect.bottom - fh.descent;

        // Truncation
        BString clipped;
        BFont font;
        parent->GetFont(&font);

        // BFont::TruncateString(const BString* in, uint32 mode, float width, BString* out)
        // or void TruncateString(BString* inOut, uint32 mode, float width)
        // Standard Haiku BFont::TruncateString takes (const BString*, uint32, float, BString*)

        font.TruncateString(&str, fTruncate, rect.Width() - 4, &clipped);

        // Alignment
        float x = rect.left + 2;
        float w = parent->StringWidth(clipped.String());

        if (Alignment() == B_ALIGN_RIGHT) {
            x = rect.right - w - 2;
        } else if (Alignment() == B_ALIGN_CENTER) {
             x = rect.left + (rect.Width() - w) / 2;
        }

        parent->DrawString(clipped.String(), BPoint(x, y));
    }
}

int SysMonStringColumn::CompareFields(BField* field1, BField* field2)
{
    SysMonStringField* f1 = static_cast<SysMonStringField*>(field1);
    SysMonStringField* f2 = static_cast<SysMonStringField*>(field2);
    return strcmp(f1->String(), f2->String());
}


// --- SysMonIntegerField ---

SysMonIntegerField::SysMonIntegerField(int32 value)
    : fValue(value)
{
}

SysMonIntegerField::~SysMonIntegerField()
{
}

void SysMonIntegerField::SetValue(int32 value)
{
    fValue = value;
}

int32 SysMonIntegerField::Value() const
{
    return fValue;
}


// --- SysMonIntegerColumn ---

SysMonIntegerColumn::SysMonIntegerColumn(const char* title, float width, float minWidth,
                  float maxWidth, alignment align)
    : BColumn(title, width, minWidth, maxWidth, align)
{
}

void SysMonIntegerColumn::DrawField(BField* field, BRect rect, BView* parent)
{
    SysMonIntegerField* intField = static_cast<SysMonIntegerField*>(field);
    if (intField) {
        BString str;
        str << intField->Value();

        font_height fh;
        parent->GetFontHeight(&fh);
        float y = rect.bottom - fh.descent;

        // Alignment
        float x = rect.left + 2;
        float w = parent->StringWidth(str.String());

        if (Alignment() == B_ALIGN_RIGHT) {
            x = rect.right - w - 2;
        } else if (Alignment() == B_ALIGN_CENTER) {
             x = rect.left + (rect.Width() - w) / 2;
        }

        parent->DrawString(str.String(), BPoint(x, y));
    }
}

int SysMonIntegerColumn::CompareFields(BField* field1, BField* field2)
{
    SysMonIntegerField* f1 = static_cast<SysMonIntegerField*>(field1);
    SysMonIntegerField* f2 = static_cast<SysMonIntegerField*>(field2);

    if (f1->Value() < f2->Value()) return -1;
    if (f1->Value() > f2->Value()) return 1;
    return 0;
}

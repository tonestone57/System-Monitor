#ifndef COLUMNTYPES_H
#define COLUMNTYPES_H

#include <SupportDefs.h>
#include <String.h>
#include <Font.h>
#include <private/interface/ColumnListView.h>

// --- SysMonStringField ---
class SysMonStringField : public BField {
public:
    SysMonStringField(const char* string);
    virtual ~SysMonStringField();

    void SetString(const char* string);
    const char* String() const;
    void SetClippedString(const char* string);
    const char* ClippedString();

private:
    BString fString;
    BString fClippedString;
};

// --- SysMonStringColumn ---
class SysMonStringColumn : public BColumn {
public:
    SysMonStringColumn(const char* title, float width, float minWidth,
                  float maxWidth, uint32 truncate, alignment align = B_ALIGN_LEFT);
    virtual void DrawField(BField* field, BRect rect, BView* parent);
    virtual int CompareFields(BField* field1, BField* field2);
private:
    uint32 fTruncate;
};

// --- SysMonIntegerField ---
class SysMonIntegerField : public BField {
public:
    SysMonIntegerField(int32 value);
    virtual ~SysMonIntegerField();

    void SetValue(int32 value);
    int32 Value() const;

private:
    int32 fValue;
};

// --- SysMonIntegerColumn ---
class SysMonIntegerColumn : public BColumn {
public:
    SysMonIntegerColumn(const char* title, float width, float minWidth,
                   float maxWidth, alignment align = B_ALIGN_RIGHT);
    virtual void DrawField(BField* field, BRect rect, BView* parent);
    virtual int CompareFields(BField* field1, BField* field2);
};

#endif // COLUMNTYPES_H

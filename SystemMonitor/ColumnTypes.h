#ifndef COLUMNTYPES_H
#define COLUMNTYPES_H

#include <ColumnTypes.h>
#include <cstdio>
#include "Utils.h"

// --- Float / Percentage Column ---

class FloatField : public BStringField {
public:
    FloatField(float value) : BStringField(""), fValue(value) {
        UpdateString();
    }
    void SetValue(float value) {
        if (fValue == value) return;
        fValue = value;
        UpdateString();
    }
    float Value() const { return fValue; }
private:
    void UpdateString() {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f", fValue);
        SetString(buffer);
    }
    float fValue;
};

class BFloatColumn : public BStringColumn {
public:
    BFloatColumn(const char* title, float width, float minWidth, float maxWidth,
                uint32 truncate, alignment align = B_ALIGN_LEFT)
        : BStringColumn(title, width, minWidth, maxWidth, truncate, align) {}

    virtual int CompareFields(BField* field1, BField* field2) {
        float val1 = ((FloatField*)field1)->Value();
        float val2 = ((FloatField*)field2)->Value();
        if (val1 < val2) return -1;
        if (val1 > val2) return 1;
        return 0;
    }
};

// --- Size (Bytes) Column ---

class SizeField : public BStringField {
public:
    SizeField(uint64 value) : BStringField(""), fValue(value) {
        UpdateString();
    }
    void SetValue(uint64 value) {
        if (fValue == value) return;
        fValue = value;
        UpdateString();
    }
    uint64 Value() const { return fValue; }
private:
    void UpdateString() {
        SetString(FormatBytes(fValue));
    }
    uint64 fValue;
};

class BSizeColumn : public BStringColumn {
public:
    BSizeColumn(const char* title, float width, float minWidth, float maxWidth,
                   uint32 truncate, alignment align = B_ALIGN_LEFT)
        : BStringColumn(title, width, minWidth, maxWidth, truncate, align) {}

    virtual int CompareFields(BField* field1, BField* field2) {
        uint64 val1 = ((SizeField*)field1)->Value();
        uint64 val2 = ((SizeField*)field2)->Value();
        if (val1 < val2) return -1;
        if (val1 > val2) return 1;
        return 0;
    }
};

// --- Speed (Bytes/sec) Column ---

class SpeedField : public BStringField {
public:
    SpeedField(uint64 value) : BStringField(""), fValue(value) {
        UpdateString();
    }
    void SetValue(uint64 value) {
        if (fValue == value) return;
        fValue = value;
        UpdateString();
    }
    uint64 Value() const { return fValue; }
private:
    void UpdateString() {
        // FormatSpeed expects (bytes, micros). To treat fValue as bytes/sec,
        // we pass (fValue, 1000000).
        SetString(FormatSpeed(fValue, 1000000));
    }
    uint64 fValue;
};

class BSpeedColumn : public BStringColumn {
public:
    BSpeedColumn(const char* title, float width, float minWidth, float maxWidth,
                   uint32 truncate, alignment align = B_ALIGN_LEFT)
        : BStringColumn(title, width, minWidth, maxWidth, truncate, align) {}

    virtual int CompareFields(BField* field1, BField* field2) {
        uint64 val1 = ((SpeedField*)field1)->Value();
        uint64 val2 = ((SpeedField*)field2)->Value();
        if (val1 < val2) return -1;
        if (val1 > val2) return 1;
        return 0;
    }
};

#endif // COLUMNTYPES_H

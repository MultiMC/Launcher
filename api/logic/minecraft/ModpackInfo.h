#pragma once

#include <QString>

struct MaybeInt {
    MaybeInt() = default;
    MaybeInt(const int value):
        hasValue(true),
        content(value)
    {};

    operator bool() const {
        return hasValue;
    }
    void operator=(int value) {
        set(value);
    }
    int operator *() const {
        return get();
    }

    void set(int value) {
        if(value == -1) {
            hasValue = false;
            content = 0;
            return;
        }
        hasValue = true;
        content = value;
    }
    int get() const {
        if(hasValue) {
            return content;
        }
        return 0;
    }
    void reset() {
        hasValue = false;
        content = 0;
    }
private:
    bool hasValue = false;
    int content = 0;
};

struct PackProfileModpackInfo
{
    operator bool() const {
        return hasValue;
    }
    bool hasValue = false;

    QString platform;
    QString repository;
    QString coordinate;

    MaybeInt minHeap;
    MaybeInt optimalHeap;
    MaybeInt maxHeap;
    QString recommendedArgs;
};

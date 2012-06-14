#include <stdint.h>
#include <stdbool.h>

#ifndef KJSON_NUMBOX_H_
#define KJSON_NUMBOX_H_

#define TagMask     (0xffff800000000000LL)
#define TagBitShift (47)
#define TagBaseMask (0x1fff0ULL)
#define TagBase     (TagBaseMask << TagBitShift)
#define TagDouble   ((TagBaseMask | 0x0ULL) << TagBitShift)
#define TagDouble2  ((TagBaseMask | 0x7ULL) << TagBitShift)
#define TagInt32    ((TagBaseMask | 0x2ULL) << TagBitShift)
#define TagBoolean  ((TagBaseMask | 0x4ULL) << TagBitShift)
#define TagNull     ((TagBaseMask | 0x6ULL) << TagBitShift)
#define TagObject   ((TagBaseMask | 0x3ULL) << TagBitShift)
#define TagString   ((TagBaseMask | 0x1ULL) << TagBitShift)
#define TagArray    ((TagBaseMask | 0x5ULL) << TagBitShift)

union JSON;
typedef union numbox {
    void    *pval;
    double   dval;
    int32_t  ival;
    bool     bval;
    uint64_t bits;
} Value;

static inline Value ValueF(double d) {
    Value v; v.dval = d; return v;
}
static inline Value ValueI(int32_t ival) {
    uint64_t n = (uint64_t)ival;
    n = n & 0x00000000ffffffffLL;
    Value v; v.bits = n | TagInt32; return v;
}
static inline Value ValueB(bool bval) {
    Value v; v.bits = (uint64_t)bval | TagBoolean; return v;
}
static inline Value ValueO(union JSON *oval) {
    Value v; v.bits = (uint64_t)oval | TagObject; return v;
}
static inline Value ValueS(union JSON *sval) {
    Value v; v.bits = (uint64_t)sval | TagString; return v;
}
static inline Value ValueA(union JSON *aval) {
    Value v; v.bits = (uint64_t)aval | TagArray; return v;
}
static inline Value ValueN() {
    Value v; v.bits = TagNull; return v;
}
static inline Value Value_T(uint64_t val, uint8_t tag) {
    Value v; v.bits = val | ((uint64_t)tag << TagBitShift); return v;
}
static inline Value toVal(void *val) {
    Value v; v.bits = (uint64_t) val; return v;
}
static inline uint64_t Tag(Value v) { return (v.bits &  TagMask); }
static inline uint64_t Val(Value v) { return (v.bits & ~TagMask); }
static inline double toDouble(Value v) {
    return v.dval;
}
static inline int32_t toInt32(Value v) {
    return (int32_t) Val(v);
}
static inline bool toBool(Value v) {
    return (bool) Val(v);
}
static inline union JSON *toObj(Value v) {
    return (union JSON*) Val(v);
}
static inline union JSON *toStr(Value v) {
    return (union JSON*) Val(v);
}
static inline union JSON *toAry(Value v) {
    return (union JSON*) Val(v);
}
static inline union JSON *toNul(Value v) {
    return (union JSON*) Val(v);
}
static inline bool IsDouble(Value v) {
    return Tag(v) <= TagDouble;
}
static inline bool IsInt32(Value v) {
    return Tag(v) == TagInt32;
}
static inline bool IsBool(Value v) {
    return Tag(v) == TagBoolean;
}
static inline bool IsObj(Value v) {
    return Tag(v) == TagObject;
}
static inline bool IsStr(Value v) {
    return Tag(v) == TagString;
}
static inline bool IsAry(Value v) {
    return Tag(v) == TagArray;
}
static inline bool IsNull(Value v) {
    return Tag(v) == TagNull;
}
#endif /* end of include guard */

///////////////////////////////////////////////////////////////////////////////
// Copyright 2016, Oushu Inc.
// All rights reserved.
//
// Author:
///////////////////////////////////////////////////////////////////////////////

#ifndef DBCOMMON_SRC_DBCOMMON_NODES_DATUM_H_
#define DBCOMMON_SRC_DBCOMMON_NODES_DATUM_H_

#include <string>

namespace dbcommon {

typedef struct {
  int64_t second;
  int64_t nanosecond;
} Timestamp;

/**
 * A structure to represent a scalar value
 */
struct Datum {
  union ValutType {
    int64_t i64;
    int32_t i32;
    int16_t i16;
    int8_t i8;

    ValutType() {}
    explicit ValutType(int64_t v) : i64(v) {}
    explicit ValutType(int32_t v) : i64(0) { i32 = v; }
    explicit ValutType(int16_t v) : i64(0) { i16 = v; }
    explicit ValutType(int8_t v) : i64(0) { i8 = v; }
  } value;

  Datum() {}
  explicit Datum(int64_t v) : value(v) {}
  explicit Datum(int32_t v) : value(v) {}
  explicit Datum(int16_t v) : value(v) {}
  explicit Datum(int8_t v) : value(v) {}
};

// Compare to datum and return true if they are equal.
// @param d1 The left datum of equal operator.
// @param d2 The right datum of equal operator.
// @return Return true if d1 is equal d2.
static inline bool operator==(const Datum &d1, const Datum &d2) {
  return d1.value.i64 == d2.value.i64;
}

// Compare to datum and return true if they are not equal.
// @param d1 The left datum of not equal operator.
// @param d2 The right datum of not equal operator.
// @return Return true if d1 is not equal d2.
static inline bool operator!=(const Datum &d1, const Datum &d2) {
  return d1.value.i64 != d2.value.i64;
}

// Declare DatumExtracter template and dispatch to different
// specialization based on data type size.
template <typename T, size_t = sizeof(T)>
struct DatumExtracter;

// Declare DatumExtracter template for 8 bit type.
template <typename T>
struct DatumExtracter<T, sizeof(int8_t)> {
  // Get primitive value of a datum.
  // @param d The datum which contains the value.
  // @return Return the 8 bit primitive value of the datum.
  static inline T DatumGetValue(const Datum &d) {
    return *reinterpret_cast<const T *>(&d.value.i8);
  }
};

// Declare DatumExtracter template for 16 bit type.
template <typename T>
struct DatumExtracter<T, sizeof(int16_t)> {
  // Get primitive value of a datum.
  // @param d The datum which contains the value.
  // @return Return the 16 bit primitive value of the datum.
  static inline T DatumGetValue(const Datum &d) {
    return *reinterpret_cast<const T *>(&d.value.i16);
  }
};

// Declare DatumExtracter template for 32 bit type.
template <typename T>
struct DatumExtracter<T, sizeof(int32_t)> {
  // Get primitive value of a datum.
  // @param d The datum which contains the value.
  // @return Return the 32 bit primitive value of the datum.
  static inline T DatumGetValue(const Datum &d) {
    return *reinterpret_cast<const T *>(&d.value.i32);
  }
};

// Declare DatumExtracter template for 64 bit type.
template <typename T>
struct DatumExtracter<T, sizeof(int64_t)> {
  // Get primitive value of a datum.
  // @param d The datum which contains the value.
  // @return Return the 64 bit primitive value of the datum.
  static inline T DatumGetValue(const Datum &d) {
    return *reinterpret_cast<const T *>(&d.value.i64);
  }
};

// Get primitive value of a datum.
// @param d The datum which contains the value.
// @return Return the primitive value of the datum.
template <typename T>
static inline T DatumGetValue(const Datum &d) {
  return DatumExtracter<T>::DatumGetValue(d);
}

// Declare DatumOperation template and dispatch to different
// specialization based on data type size.
template <typename T, size_t = sizeof(T)>
struct DatumOperation;

template <typename T>
struct DatumOperation<T, sizeof(int64_t)> {
  static inline void DatumPlus(Datum &d, T v) {
    *reinterpret_cast<T *>(&d.value.i64) =
        *reinterpret_cast<T *>(&d.value.i64) + v;
  }
};

template <typename T>
struct DatumOperation<T, sizeof(int32_t)> {
  static inline void DatumPlus(Datum &d, T v) {
    *reinterpret_cast<T *>(&d.value.i32) =
        *reinterpret_cast<T *>(&d.value.i32) + v;
  }
};

template <typename T>
struct DatumOperation<T, sizeof(int16_t)> {
  static inline void DatumPlus(Datum &d, T v) {
    *reinterpret_cast<T *>(&d.value.i16) =
        *reinterpret_cast<T *>(&d.value.i16) + v;
  }
};

template <typename T>
struct DatumOperation<T, sizeof(int8_t)> {
  static inline void DatumPlus(Datum &d, T v) {
    *reinterpret_cast<T *>(&d.value.i8) =
        *reinterpret_cast<T *>(&d.value.i8) + v;
  }
};

template <typename T>
static inline void DatumPlus(Datum &d, T v) {  // NOLINT
  DatumOperation<T>::DatumPlus(d, v);
}

// Declare DatumCreater template and dispatch to different
// specialization based on data type size.
template <typename T, size_t = sizeof(T)>
struct DatumCreater;

// Declare DatumCreate template for 8 bit type.
template <typename T>
struct DatumCreater<T, sizeof(int8_t)> {
  // Create a datum with given 8 bit primitive type.
  // @param value The initial value of the datum.
  // @return Return a new datum.
  static inline Datum CreateDatum(const T &value) {
    return Datum(*reinterpret_cast<const int8_t *>(&value));
  }
};

// Declare DatumCreate template for 16 bit type.
template <typename T>
struct DatumCreater<T, sizeof(int16_t)> {
  // Create a datum with given 16 bit primitive type.
  // @param value The initial value of the datum.
  // @return Return a new datum.
  static inline Datum CreateDatum(const T &value) {
    return Datum(*reinterpret_cast<const int16_t *>(&value));
  }
};

// Declare DatumCreate template for 32 bit type.
template <typename T>
struct DatumCreater<T, sizeof(int32_t)> {
  // Create a datum with given 32 bit primitive type.
  // @param value The initial value of the datum.
  // @return Return a new datum.
  static inline Datum CreateDatum(const T &value) {
    return Datum(*reinterpret_cast<const int32_t *>(&value));
  }
};

// Declare DatumCreate template for 64 bit type.
template <typename T>
struct DatumCreater<T, sizeof(int64_t)> {
  // Create a datum with given 64 bit primitive type.
  // @param value The initial value of the datum.
  // @return Return a new datum.
  static inline Datum CreateDatum(const T &value) {
    return Datum(*reinterpret_cast<const int64_t *>(&value));
  }
};

// Declare DatumCreate template for std::string.
template <typename T>
struct DatumCreater<T, sizeof(std::string)> {
  // Create a datum with given std::string.
  // @param value The initial value of the datum.
  // @return Return a new datum.
  static inline Datum CreateDatum(const T &value) {
    return Datum(reinterpret_cast<int64_t>(value.c_str()));
  }
};

// Create a datum with given primitive type.
// @param value The initial value of the datum.
// @return Return a new datum.
template <typename T>
static inline Datum CreateDatum(T const &value) {
  return DatumCreater<typename std::remove_cv<T>::type>::CreateDatum(value);
}

// Create a datum with the given data type ID.
// @param str The string representation of the data.
// @param typeId The data type ID of the return datum.
// @return Return a new datum.
Datum CreateDatum(const char *str, int typeId);

Datum CreateDatum(const char *str, Timestamp *timestamp, int typeId);

// Copy a datum, if the datum is pass by reference,
//   copy the content of the datum.
// @param d The datum to be copied.
// @param typeId The data type ID of the datum.
// @return Return the new datum.
Datum CopyDatum(const Datum &d, int typeId);

// Convert datum to a string.
// @param d The datum to be convert.
// @param typeId The data type ID of datum.
// @return Return the string of datum.
std::string DatumToString(const Datum &d, int typeId);

// Dump datum to buffer as binary.
// @param d The datum to be convert.
// @param typeId The data type ID of datum.
// @return Return the binary of datum.
std::string DatumToBinary(const Datum &d, int typeId);

}  // namespace dbcommon.

#endif  // DBCOMMON_SRC_DBCOMMON_NODES_DATUM_H_

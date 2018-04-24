/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// See docs in ../ops/io_ops.cc.
#include <memory>
#include "dbcommon/filesystem/file-system-manager.h"
#include "storage/format/format.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/example/feature.pb.h"
#include "tensorflow/core/framework/reader_base.h"
#include "tensorflow/core/framework/reader_op_kernel.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/lib/core/coding.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/env.h"

namespace tensorflow {
class OrcRowReader : public ReaderBase {
 public:
  OrcRowReader(const string& node_name, const string& compression_type,
               Env* env)
      : ReaderBase(strings::StrCat("OrcRowReader '", node_name, "'")),
        env_(env),
        offset_(0),
        compression_type_(compression_type) {
    params_.set("number.tuples.per.batch", std::to_string(batch_size_));
    params_.set("table.options", "{\"compresstype\":\"" + compression_type_ +
                                     "\",\"rlecoder\":\"v2\","
                                     "\"bloomfilter\":\"0\"}");
    format_ = storage::Format::createFormat(univplan::ORC_FORMAT, &params_);
    format_->setFileSystemManager(&FSManager_);
  }

  Status OnWorkStartedLocked() override {
    offset_ = 0;
    string protocol, path;
    getPathFromUrl(current_work(), protocol, path);
    string fullFileName = protocol + path;
    dbcommon::FileSystem* fs = FSManager_.get(fullFileName.c_str());
    if (!fs->exists(path.c_str())) return errors::NotFound(path.c_str());

    std::vector<storage::Input::uptr> files;
    dbcommon::FileInfo::uptr info = fs->getFileInfo(path.c_str());
    storage::Input::uptr file(
        new storage::FileInput(fullFileName.c_str(), info->size));
    files.push_back(std::move(file));
    tasks_ = format_->createTasks(files, 1);
    const univplan::UnivPlanScanTask& t = tasks_->Get(0);
    format_->beginScan(string(""), &(t.splits()), nullptr, nullptr, nullptr,
                       false);
    return Status::OK();
  }

  Status OnWorkFinishedLocked() override {
    format_->endScan();
    format_.reset(nullptr);
    result_.reset(nullptr);
    tasks_.reset(nullptr);
    return Status::OK();
  }

  Status ReadLocked(string* key, string* value, bool* produced,
                    bool* at_end) override {
    if (offset_ % batch_size_ == 0) {
      result_ = format_->next();  // read a batch data
    }
    if (result_ != nullptr) {
      const dbcommon::TupleBatchReader& reader = result_->getTupleBatchReader();
      uint64 rowId = offset_ % batch_size_;
      if (rowId < result_->getNumOfRows()) {
        // orcStringEncode(reader, rowId, value);
        orcExample(reader, rowId, value);
        *key = strings::StrCat(current_work(), ":", offset_++);
        *produced = true;
        return Status::OK();
      } else {  // out of range if TupleBatch get data less than baztch size
        *at_end = true;
        return Status::OK();
      }
    } else {  // cannot get next bacth
      *at_end = true;
      return Status::OK();
    }
    return Status::OK();
  }

  Status ResetLocked() override {
    offset_ = 0;
    rowId_ = 0;
    format_.reset(nullptr);
    result_.reset(nullptr);
    tasks_.reset(nullptr);
    return ReaderBase::ResetLocked();
  }

 private:
  Env* const env_;
  uint64 offset_;
  uint64 rowId_;
  const int batch_size_ = 32;
  string compression_type_;
  dbcommon::FileSystemManager FSManager_;
  dbcommon::Parameters params_;
  dbcommon::TupleBatch::uptr result_;
  std::unique_ptr<storage::Format> format_;
  std::unique_ptr<univplan::UnivPlanScanTaskList> tasks_;

  void orcStringEncode(const dbcommon::TupleBatchReader& reader, uint64_t rowId,
                       string* value) {
    if (result_ != nullptr) {
      uint64_t colCount = result_->getNumOfColumns();
      core::PutFixed64(value, colCount);
      for (uint64_t i = 0; i < colCount; i++) {
        auto kind = reader[i]->getTypeKind();
        switch (kind) {
            // integer
          case dbcommon::BOOLEANID:
            core::PutFixed32(value, DT_BOOL);
            break;
          case dbcommon::TINYINTID:
            core::PutFixed32(value, DT_INT8);
            break;
          case dbcommon::SMALLINTID:
            core::PutFixed32(value, DT_INT16);
            break;
          case dbcommon::INTID:
            core::PutFixed32(value, DT_INT32);
            break;
          case dbcommon::BIGINTID:
            core::PutFixed32(value, DT_INT64);
            break;
            // float
          case dbcommon::FLOATID:
            core::PutFixed32(value, DT_FLOAT);
            break;
          case dbcommon::DOUBLEID:
            core::PutFixed32(value, DT_DOUBLE);
            break;
            // date
          case dbcommon::DATEID:
            core::PutFixed32(value, DT_INT32);
            break;
          case dbcommon::TIMESTAMPID:
          case dbcommon::TIMEID:
            core::PutFixed32(value, DT_INT64);
            break;
            // string
          case dbcommon::STRINGID:
            core::PutFixed32(value, DT_STRING);
            break;
          case dbcommon::VARCHARID:
            core::PutFixed32(value, DT_STRING);
            break;
          case dbcommon::CHARID:
            core::PutFixed32(value, DT_STRING);
            break;

          default:
            core::PutFixed32(value, -1);
            std::cout << "Can not encode type!" << std::endl;
            continue;
        }

        uint64_t len;
        bool isNull;
        if (kind == dbcommon::TIMESTAMPID) {
          len = sizeof(int64_t);
          const char* data =
              static_cast<dbcommon::TimestampVector*>(reader[i].get())
                  ->readValue(rowId, &isNull);
          core::PutFixed64(value, len);
          value->append(data, len);
        } else {
          const char* data = reader[i]->read(rowId, &len, &isNull);
          core::PutFixed64(value, len);
          value->append(data, len);
        }
      }
    }
  }

  void AddInt64Feature(Features* features, string key, int64_t value) {
    Feature feature;
    feature.mutable_int64_list()->add_value(value);
    auto map = features->mutable_feature();
    (*map)[key] = feature;
  }

  void AddFloatFeature(Features* features, string key, float value) {
    Feature feature;
    feature.mutable_float_list()->add_value(value);
    auto map = features->mutable_feature();
    (*map)[key] = feature;
  }

  void AddBytesFeature(Features* features, string key, string value) {
    Feature feature;
    feature.mutable_bytes_list()->add_value(value);
    auto map = features->mutable_feature();
    (*map)[key] = feature;
  }

  void orcExample(const dbcommon::TupleBatchReader& reader, uint64_t rowId,
                  string* value) {
    if (result_ != nullptr) {
      Example example;
      Features* features = example.mutable_features();
      uint64_t colCount = result_->getNumOfColumns();
      for (uint64_t i = 0; i < colCount; i++) {
        auto kind = reader[i]->getTypeKind();
        uint64_t len;
        bool isNull;
        switch (kind) {
            // integer
          case dbcommon::BOOLEANID: {
            bool data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(bool));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, static_cast<int64_t>(data));
            break;
          }
          case dbcommon::TINYINTID: {
            int8_t data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(int8_t));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, static_cast<int64_t>(data));
            break;
          }
          case dbcommon::SMALLINTID: {
            int16_t data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(int16_t));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, static_cast<int64_t>(data));
            break;
          }
          case dbcommon::INTID: {
            int32_t data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(int32_t));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, static_cast<int64_t>(data));
            break;
          }
          case dbcommon::BIGINTID: {
            int64_t data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(int64_t));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, data);
            break;
          }
          case dbcommon::DATEID: {
            int32_t data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(int32_t));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, static_cast<int64_t>(data));
            break;
          }
          case dbcommon::TIMESTAMPID: {
            int64_t data;
            const char* databuf =
                static_cast<dbcommon::TimestampVector*>(reader[i].get())
                    ->readValue(rowId, &isNull);
            memcpy(&data, databuf, sizeof(int64_t));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, data);
            break;
          }
          case dbcommon::TIMEID: {
            int64_t data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(int64_t));
            string key = strings::StrCat("key", i);
            AddInt64Feature(features, key, data);
            break;
          }
            // float
          case dbcommon::FLOATID: {
            float data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(float));
            string key = strings::StrCat("key", i);
            AddFloatFeature(features, key, data);
            break;
          }
          case dbcommon::DOUBLEID: {
            double data;
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            memcpy(&data, databuf, sizeof(double));
            string key = strings::StrCat("key", i);
            AddFloatFeature(features, key, static_cast<float>(data));
            break;
          }
            // string
          case dbcommon::STRINGID:
          case dbcommon::VARCHARID:
          case dbcommon::CHARID: {
            const char* databuf = reader[i]->read(rowId, &len, &isNull);
            string key = strings::StrCat("key", i);
            string value;
            value.append(databuf, len);
            AddBytesFeature(features, key, value);
            break;
          }

          default:
            std::cout << "Can not encode type!" << std::endl;
            continue;
        }
      }
      *value = example.SerializeAsString();
    }
  }

  void getPathFromUrl(string url, string& protocol, string& path) {
    string protocolEnd = "://";
    int pos = url.find(protocolEnd);
    if (pos < 0) {  // local file system
      protocol = "file://";
      path = url;
    } else {  // url
      protocol = url.substr(0, pos + protocolEnd.size());
      if (protocol == "file://") {
        path = url.substr(pos + protocolEnd.size() - 1, url.size() - pos);
      } else {
        pos = url.find('/', pos + protocolEnd.size());
        protocol = url.substr(0, pos);
        path = url.substr(pos, url.size() - pos);
      }
    }
  }
};

class OrcRowReaderOp : public ReaderOpKernel {
 public:
  explicit OrcRowReaderOp(OpKernelConstruction* context)
      : ReaderOpKernel(context) {
    Env* env = context->env();

    string compression_type;
    OP_REQUIRES_OK(context,
                   context->GetAttr("compression_type", &compression_type));
    OP_REQUIRES(
        context,
        compression_type == "snappy" || compression_type == "lz4" ||
            compression_type == "none",
        errors::InvalidArgument("compression_type must be snappy, lz4 or none",
                                compression_type));
    SetReaderFactory([this, compression_type, env]() {
      return new OrcRowReader(name(), compression_type, env);
    });
  }
};

REGISTER_KERNEL_BUILDER(Name("OrcRowReader").Device(DEVICE_CPU),
                        OrcRowReaderOp);
REGISTER_KERNEL_BUILDER(Name("OrcRowReaderV2").Device(DEVICE_CPU),
                        OrcRowReaderOp);
}  // namespace tensorflow
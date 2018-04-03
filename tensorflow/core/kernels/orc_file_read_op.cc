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

#include <iostream>
#include <memory>
#include "dbcommon/filesystem/file-system-manager.h"
#include "storage/format/format.h"
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
  OrcRowReader(const string& node_name, Env* env)
      : ReaderBase(strings::StrCat("OrcRowReader '", node_name, "'")),
        env_(env),
        offset_(0) {
    params_.set("number.tuples.per.batch", std::to_string(batch_size_));
    params_.set("table.options",
                "{\"compresstype\":\"snappy\",\"rlecoder\":\"v2\","
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
    std::vector<storage::Input::uptr> files;
    dbcommon::FileInfo::uptr info = fs->getFileInfo(path.c_str());
    storage::Input::uptr file(
        new storage::FileInput(fullFileName.c_str(), info->size));
    files.push_back(std::move(file));
    tasks_ = format_->createTasks(files, 1);
    const univplan::UnivPlanScanTask& t = tasks_->Get(0);
    format_->beginScan(string(""), &(t.splits()), nullptr, nullptr, nullptr, false);
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
        orcStringEncode(reader, rowId, value);
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
  uint64 batch_size_ = 32;
  dbcommon::FileSystemManager FSManager_;
  dbcommon::Parameters params_;
  std::unique_ptr<storage::Format> format_;
  std::unique_ptr<univplan::UnivPlanScanTaskList> tasks_;
  dbcommon::TupleBatch::uptr result_;

  void orcStringEncode(const dbcommon::TupleBatchReader& reader, uint64_t rowId,
                       string* value) {
    if (result_ != nullptr) {
      uint64_t colCount = result_->getNumOfColumns();
      core::PutFixed64(value,colCount);
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
        const char* data = reader[i]->read(rowId, &len, &isNull);
        core::PutFixed64(value, len);
        value->append(data, len);
      }
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

    SetReaderFactory([this, env]() { return new OrcRowReader(name(), env); });
  }
};

REGISTER_KERNEL_BUILDER(Name("OrcRowReader").Device(DEVICE_CPU),
                        OrcRowReaderOp);
REGISTER_KERNEL_BUILDER(Name("OrcRowReaderV2").Device(DEVICE_CPU),
                        OrcRowReaderOp);
}  // namespace tensorflow
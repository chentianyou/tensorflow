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
#include "tensorflow/core/framework/reader_base.h"
#include "tensorflow/core/framework/reader_op_kernel.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/io/inputbuffer.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/env.h"
#include "orc/Reader.hh"

namespace tensorflow {
class OrcRowReader : public ReaderBase {
public:
  OrcRowReader(const string& node_name, const string& compression_type,
                 Env* env)
      : ReaderBase(strings::StrCat("OrcRowReader '", node_name, "'")),
        env_(env),
        offset_(0),
        compression_type_(compression_type) {}

  Status OnWorkStartedLocked() override {
    offset_ = 0;
    TF_RETURN_IF_ERROR(env_->NewRandomAccessFile(current_work(), &file_));
    
    return Status::OK();
  }

  Status OnWorkFinishedLocked() override {
    return Status::OK();
  }

  Status ReadLocked(string* key, string* value, bool* produced,
                    bool* at_end) override {
    return Status::OK();
  }

  Status ResetLocked() override {
    return ReaderBase::ResetLocked();
  }

  // TODO(josh11b): Implement serializing and restoring the state.

 private:
  Env* const env_;
  uint64 offset_;
  std::unique_ptr<RandomAccessFile> file_;
  std::unique_ptr<orc::Reader> reader_;
  string compression_type_ = "";
};

class OrcRowReaderOp : public ReaderOpKernel {
 public:
  explicit OrcRowReaderOp(OpKernelConstruction* context)
      : ReaderOpKernel(context) {
    Env* env = context->env();

    string compression_type;
    OP_REQUIRES_OK(context,
                   context->GetAttr("compression_type", &compression_type));

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
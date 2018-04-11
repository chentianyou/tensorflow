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

// See docs in ../ops/parsing_ops.cc.
#include <vector>

#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/lib/core/errors.h"

namespace tensorflow {

class DecodeORCOp : public OpKernel {
 public:
  explicit DecodeORCOp(OpKernelConstruction* ctx) : OpKernel(ctx) {
    OP_REQUIRES_OK(ctx, ctx->GetAttr("OUT_TYPE", &out_type_));
    OP_REQUIRES(ctx, out_type_.size() < std::numeric_limits<int>::max(),
                errors::InvalidArgument("Out type too large"));
  }

  void Compute(OpKernelContext* ctx) override {
    const Tensor* records;

    OP_REQUIRES_OK(ctx, ctx->input("records", &records));

    auto records_t = records->flat<string>();
    int64 records_size = records_t.size();

    OpOutputList output;
    OP_REQUIRES_OK(ctx, ctx->output_list("output", &output));

    for (int i = 0; i < static_cast<int>(out_type_.size()); ++i) {
      Tensor* out = nullptr;
      OP_REQUIRES_OK(ctx, output.allocate(i, records->shape(), &out));
    }

    for (int64 i = 0; i < records_size; ++i) {
      const StringPiece record(records_t(i));
      std::vector<string> fields;
      ExtractOrcFields(ctx, record, &fields);
      OP_REQUIRES(ctx, fields.size() == out_type_.size(),
                  errors::InvalidArgument("Expect ", out_type_.size(),
                                          " fields but have ", fields.size(),
                                          " in record ", i));
      // Check each field in the record
      for (int f = 0; f < static_cast<int>(out_type_.size()); ++f) {
        const DataType& dtype = out_type_[f];
        switch (dtype) {
          case DT_BOOL: {
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));
              output[f]->flat<bool>()(i) = false;
            } else {
              bool value;
              memcpy(&value, fields[f].data(), sizeof(bool));
              output[f]->flat<bool>()(i) = value;
            }
            break;
          }
          case DT_INT8: {
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));
              output[f]->flat<int8>()(i) = 0;
            } else {
              int8 value;
              memcpy(&value, fields[f].data(), sizeof(int8));
              output[f]->flat<int8>()(i) = value;
            }
            break;
          }
          case DT_INT16: {
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));
              output[f]->flat<int16>()(i) = 0;
            } else {
              int16 value;
              memcpy(&value, fields[f].data(), sizeof(int16));
              output[f]->flat<int16>()(i) = value;
            }
          }
          case DT_INT32: {
            // If this field is empty or NA value, check if default is given:
            // If yes, use default value; Otherwise report error.
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));
              output[f]->flat<int32>()(i) = 0;
            } else {
              int32 value;
              memcpy(&value, fields[f].data(), sizeof(int32));
              output[f]->flat<int32>()(i) = value;
            }
            break;
          }
          case DT_INT64: {
            // If this field is empty or NA value, check if default is given:
            // If yes, use default value; Otherwise report error.
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));

              output[f]->flat<int64>()(i) = 0;
            } else {
              int64 value;
              memcpy(&value, fields[f].data(), sizeof(int64));
              output[f]->flat<int64>()(i) = value;
            }
            break;
          }
          case DT_FLOAT: {
            // If this field is empty or NA value, check if default is given:
            // If yes, use default value; Otherwise report error.
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));
              output[f]->flat<float>()(i) = 0.0f;
            } else {
              float value;
              memcpy(&value, fields[f].data(), sizeof(float));
              output[f]->flat<float>()(i) = value;
            }
            break;
          }
          case DT_DOUBLE: {
            // If this field is empty or NA value, check if default is given:
            // If yes, use default value; Otherwise report error.
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));
              output[f]->flat<double>()(i) = 0.0;
            } else {
              double value;
              memcpy(&value, fields[f].data(), sizeof(double));
              output[f]->flat<double>()(i) = value;
            }
            break;
          }
          case DT_STRING: {
            // If this field is empty or NA value, check if default is given:
            // If yes, use default value; Otherwise report error.
            if (fields[f].empty()) {
              OP_REQUIRES(ctx, true,
                          errors::InvalidArgument(
                              "Field ", f,
                              " is required but missing in record ", i, "!"));
              output[f]->flat<string>()(i) = "\0";
            } else {
              output[f]->flat<string>()(i) = fields[f];
            }
            break;
          }

          default:
            OP_REQUIRES(ctx, false,
                        errors::InvalidArgument("csv: data type ", dtype,
                                                " not supported in field ", f));
        }
      }
    }
  }

 private:
  std::vector<DataType> out_type_;

  void ExtractOrcFields(OpKernelContext* ctx, StringPiece input,
                        std::vector<string>* result) {
    if (!input.empty()) {
      int offset = 0;
      uint64 columnCount;
      memcpy(&columnCount, input.data(), sizeof(uint64));
      offset += sizeof(uint64);

      for (uint64 i = 0; i < columnCount; i++) {
        string value;
        // get type
        uint32 kind;
        memcpy(&kind, input.data() + offset, sizeof(uint32));
        offset += sizeof(uint32);
        if (kind == -1) {  // invalid data
          value.append("\0");
          value.resize(1);
          result->push_back(value);
          continue;
        }

        uint64 length;
        memcpy(&length, input.data() + offset, sizeof(uint64));
        offset += sizeof(uint64);
        char data[length];
        memcpy(data, input.data() + offset, length);
        offset += length;
        value.append(data);
        value.resize(length);
        result->push_back(value);
      }
    }
  }
};

REGISTER_KERNEL_BUILDER(Name("DecodeORC").Device(DEVICE_CPU), DecodeORCOp);

}  // namespace tensorflow
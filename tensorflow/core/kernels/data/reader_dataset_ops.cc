/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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
#include "dbcommon/filesystem/file-system-manager.h"
#include "storage/format/format.h"
#include "tensorflow/core/example/example.pb.h"
#include "tensorflow/core/example/feature.pb.h"
#include "tensorflow/core/framework/partial_tensor_shape.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/kernels/data/dataset.h"
#include "tensorflow/core/lib/io/buffered_inputstream.h"
#include "tensorflow/core/lib/io/inputbuffer.h"
#include "tensorflow/core/lib/io/random_inputstream.h"
#include "tensorflow/core/lib/io/record_reader.h"
#include "tensorflow/core/lib/io/zlib_compression_options.h"
#include "tensorflow/core/lib/io/zlib_inputstream.h"

namespace tensorflow {

namespace {

// See documentation in ../ops/dataset_ops.cc for a high-level
// description of the following ops.

class TextLineDatasetOp : public DatasetOpKernel {
 public:
  using DatasetOpKernel::DatasetOpKernel;

  void MakeDataset(OpKernelContext* ctx, DatasetBase** output) override {
    const Tensor* filenames_tensor;
    OP_REQUIRES_OK(ctx, ctx->input("filenames", &filenames_tensor));
    OP_REQUIRES(
        ctx, filenames_tensor->dims() <= 1,
        errors::InvalidArgument("`filenames` must be a scalar or a vector."));

    string compression_type;
    OP_REQUIRES_OK(ctx, ParseScalarArgument<string>(ctx, "compression_type",
                                                    &compression_type));

    int64 buffer_size = -1;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "buffer_size", &buffer_size));
    OP_REQUIRES(
        ctx, buffer_size >= 0,
        errors::InvalidArgument("`buffer_size` must be >= 0 (0 == default)"));

    io::ZlibCompressionOptions zlib_compression_options =
        io::ZlibCompressionOptions::DEFAULT();
    if (compression_type == "ZLIB") {
      zlib_compression_options = io::ZlibCompressionOptions::DEFAULT();
    } else if (compression_type == "GZIP") {
      zlib_compression_options = io::ZlibCompressionOptions::GZIP();
    } else {
      OP_REQUIRES(ctx, compression_type.empty(),
                  errors::InvalidArgument("Unsupported compression_type."));
    }

    if (buffer_size != 0) {
      // Set the override size.
      zlib_compression_options.input_buffer_size = buffer_size;
    }

    std::vector<string> filenames;
    filenames.reserve(filenames_tensor->NumElements());
    for (int i = 0; i < filenames_tensor->NumElements(); ++i) {
      filenames.push_back(filenames_tensor->flat<string>()(i));
    }

    *output = new Dataset(ctx, std::move(filenames), compression_type,
                          zlib_compression_options);
  }

 private:
  class Dataset : public GraphDatasetBase {
   public:
    Dataset(OpKernelContext* ctx, std::vector<string> filenames,
            const string& compression_type,
            const io::ZlibCompressionOptions& options)
        : GraphDatasetBase(ctx),
          filenames_(std::move(filenames)),
          compression_type_(compression_type),
          use_compression_(!compression_type.empty()),
          options_(options) {}

    std::unique_ptr<IteratorBase> MakeIterator(
        const string& prefix) const override {
      return std::unique_ptr<IteratorBase>(
          new Iterator({this, strings::StrCat(prefix, "::TextLine")}));
    }

    const DataTypeVector& output_dtypes() const override {
      static DataTypeVector* dtypes = new DataTypeVector({DT_STRING});
      return *dtypes;
    }

    const std::vector<PartialTensorShape>& output_shapes() const override {
      static std::vector<PartialTensorShape>* shapes =
          new std::vector<PartialTensorShape>({{}});
      return *shapes;
    }

    string DebugString() override { return "TextLineDatasetOp::Dataset"; }

   protected:
    Status AsGraphDefInternal(DatasetGraphDefBuilder* b,
                              Node** output) const override {
      Node* filenames = nullptr;
      Node* compression_type = nullptr;
      Node* buffer_size = nullptr;
      TF_RETURN_IF_ERROR(b->AddVector(filenames_, &filenames));
      TF_RETURN_IF_ERROR(b->AddScalar(compression_type_, &compression_type));
      TF_RETURN_IF_ERROR(
          b->AddScalar(options_.input_buffer_size, &buffer_size));
      TF_RETURN_IF_ERROR(b->AddDataset(
          this, {filenames, compression_type, buffer_size}, output));
      return Status::OK();
    }

   private:
    class Iterator : public DatasetIterator<Dataset> {
     public:
      explicit Iterator(const Params& params)
          : DatasetIterator<Dataset>(params) {}

      Status GetNextInternal(IteratorContext* ctx,
                             std::vector<Tensor>* out_tensors,
                             bool* end_of_sequence) override {
        mutex_lock l(mu_);
        do {
          // We are currently processing a file, so try to read the next line.
          if (buffered_input_stream_) {
            string line_contents;
            Status s = buffered_input_stream_->ReadLine(&line_contents);

            if (s.ok()) {
              // Produce the line as output.
              Tensor line_tensor(ctx->allocator({}), DT_STRING, {});
              line_tensor.scalar<string>()() = line_contents;
              out_tensors->emplace_back(std::move(line_tensor));
              *end_of_sequence = false;
              return Status::OK();
            } else if (!errors::IsOutOfRange(s)) {
              // Report non-EOF errors to the caller.
              return s;
            }
            // We have reached the end of the current file, so maybe
            // move on to next file.
            ResetStreamsLocked();
            ++current_file_index_;
          }

          // Iteration ends when there are no more files to process.
          if (current_file_index_ == dataset()->filenames_.size()) {
            *end_of_sequence = true;
            return Status::OK();
          }

          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
        } while (true);
      }

     protected:
      Status SaveInternal(IteratorStateWriter* writer) override {
        mutex_lock l(mu_);
        TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("current_file_index"),
                                               current_file_index_));

        // `buffered_input_stream_` is empty if
        // 1. GetNext has not been called even once.
        // 2. All files have been read and iterator has been exhausted.
        if (buffered_input_stream_) {
          TF_RETURN_IF_ERROR(writer->WriteScalar(
              full_name("current_pos"), buffered_input_stream_->Tell()));
        }
        return Status::OK();
      }

      Status RestoreInternal(IteratorContext* ctx,
                             IteratorStateReader* reader) override {
        mutex_lock l(mu_);
        ResetStreamsLocked();
        int64 current_file_index;
        TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("current_file_index"),
                                              &current_file_index));
        current_file_index_ = size_t(current_file_index);
        // The key "current_pos" is written only if the iterator was saved
        // with an open file.
        if (reader->Contains(full_name("current_pos"))) {
          int64 current_pos;
          TF_RETURN_IF_ERROR(
              reader->ReadScalar(full_name("current_pos"), &current_pos));

          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
          TF_RETURN_IF_ERROR(buffered_input_stream_->Seek(current_pos));
        }
        return Status::OK();
      }

     private:
      // Sets up reader streams to read from the file at `current_file_index_`.
      Status SetupStreamsLocked(Env* env) EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        if (current_file_index_ >= dataset()->filenames_.size()) {
          return errors::InvalidArgument(
              "current_file_index_:", current_file_index_,
              " >= filenames_.size():", dataset()->filenames_.size());
        }

        // Actually move on to next file.
        TF_RETURN_IF_ERROR(env->NewRandomAccessFile(
            dataset()->filenames_[current_file_index_], &file_));
        input_stream_.reset(
            new io::RandomAccessInputStream(file_.get(), false));

        if (dataset()->use_compression_) {
          zlib_input_stream_.reset(new io::ZlibInputStream(
              input_stream_.get(), dataset()->options_.input_buffer_size,
              dataset()->options_.input_buffer_size, dataset()->options_));
          buffered_input_stream_.reset(new io::BufferedInputStream(
              zlib_input_stream_.get(), dataset()->options_.input_buffer_size,
              false));
        } else {
          buffered_input_stream_.reset(new io::BufferedInputStream(
              input_stream_.get(), dataset()->options_.input_buffer_size,
              false));
        }
        return Status::OK();
      }

      // Resets all reader streams.
      void ResetStreamsLocked() EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        input_stream_.reset();
        zlib_input_stream_.reset();
        buffered_input_stream_.reset();
        file_.reset();
      }

      mutex mu_;
      std::unique_ptr<io::RandomAccessInputStream> input_stream_
          GUARDED_BY(mu_);
      std::unique_ptr<io::ZlibInputStream> zlib_input_stream_ GUARDED_BY(mu_);
      std::unique_ptr<io::BufferedInputStream> buffered_input_stream_
          GUARDED_BY(mu_);
      size_t current_file_index_ GUARDED_BY(mu_) = 0;
      std::unique_ptr<RandomAccessFile> file_
          GUARDED_BY(mu_);  // must outlive input_stream_
    };

    const std::vector<string> filenames_;
    const string compression_type_;
    const bool use_compression_;
    const io::ZlibCompressionOptions options_;
  };
};

REGISTER_KERNEL_BUILDER(Name("TextLineDataset").Device(DEVICE_CPU),
                        TextLineDatasetOp);

class FixedLengthRecordDatasetOp : public DatasetOpKernel {
 public:
  using DatasetOpKernel::DatasetOpKernel;

  void MakeDataset(OpKernelContext* ctx, DatasetBase** output) override {
    const Tensor* filenames_tensor;
    OP_REQUIRES_OK(ctx, ctx->input("filenames", &filenames_tensor));
    OP_REQUIRES(
        ctx, filenames_tensor->dims() <= 1,
        errors::InvalidArgument("`filenames` must be a scalar or a vector."));

    std::vector<string> filenames;
    filenames.reserve(filenames_tensor->NumElements());
    for (int i = 0; i < filenames_tensor->NumElements(); ++i) {
      filenames.push_back(filenames_tensor->flat<string>()(i));
    }

    int64 header_bytes = -1;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "header_bytes", &header_bytes));
    OP_REQUIRES(ctx, header_bytes >= 0,
                errors::InvalidArgument("`header_bytes` must be >= 0"));

    int64 record_bytes = -1;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "record_bytes", &record_bytes));
    OP_REQUIRES(ctx, record_bytes > 0,
                errors::InvalidArgument("`record_bytes` must be > 0"));

    int64 footer_bytes = -1;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "footer_bytes", &footer_bytes));
    OP_REQUIRES(ctx, footer_bytes >= 0,
                errors::InvalidArgument("`footer_bytes` must be >= 0"));

    int64 buffer_size = -1;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "buffer_size", &buffer_size));
    OP_REQUIRES(ctx, buffer_size >= 0,
                errors::InvalidArgument("`buffer_size` must be >= 0"));
    if (buffer_size == 0) {
      buffer_size = 256 << 10;  // 256 kB as default.
    }

    *output = new Dataset(ctx, std::move(filenames), header_bytes, record_bytes,
                          footer_bytes, buffer_size);
  }

 private:
  class Dataset : public GraphDatasetBase {
   public:
    explicit Dataset(OpKernelContext* ctx, std::vector<string> filenames,
                     int64 header_bytes, int64 record_bytes, int64 footer_bytes,
                     int64 buffer_size)
        : GraphDatasetBase(ctx),
          filenames_(std::move(filenames)),
          header_bytes_(header_bytes),
          record_bytes_(record_bytes),
          footer_bytes_(footer_bytes),
          buffer_size_(buffer_size) {}

    std::unique_ptr<IteratorBase> MakeIterator(
        const string& prefix) const override {
      return std::unique_ptr<IteratorBase>(
          new Iterator({this, strings::StrCat(prefix, "::FixedLengthRecord")}));
    }

    const DataTypeVector& output_dtypes() const override {
      static DataTypeVector* dtypes = new DataTypeVector({DT_STRING});
      return *dtypes;
    }

    const std::vector<PartialTensorShape>& output_shapes() const override {
      static std::vector<PartialTensorShape>* shapes =
          new std::vector<PartialTensorShape>({{}});
      return *shapes;
    }

    string DebugString() override {
      return "FixedLengthRecordDatasetOp::Dataset";
    }

   protected:
    Status AsGraphDefInternal(DatasetGraphDefBuilder* b,
                              Node** output) const override {
      Node* filenames = nullptr;
      Node* header_bytes = nullptr;
      Node* record_bytes = nullptr;
      Node* footer_bytes = nullptr;
      Node* buffer_size = nullptr;
      TF_RETURN_IF_ERROR(b->AddVector(filenames_, &filenames));
      TF_RETURN_IF_ERROR(b->AddScalar(header_bytes_, &header_bytes));
      TF_RETURN_IF_ERROR(b->AddScalar(record_bytes_, &record_bytes));
      TF_RETURN_IF_ERROR(b->AddScalar(footer_bytes_, &footer_bytes));
      TF_RETURN_IF_ERROR(b->AddScalar(buffer_size_, &buffer_size));
      TF_RETURN_IF_ERROR(b->AddDataset(
          this,
          {filenames, header_bytes, record_bytes, footer_bytes, buffer_size},
          output));
      return Status::OK();
    }

   private:
    class Iterator : public DatasetIterator<Dataset> {
     public:
      explicit Iterator(const Params& params)
          : DatasetIterator<Dataset>(params) {}

      Status GetNextInternal(IteratorContext* ctx,
                             std::vector<Tensor>* out_tensors,
                             bool* end_of_sequence) override {
        mutex_lock l(mu_);
        do {
          // We are currently processing a file, so try to read the next record.
          if (input_buffer_) {
            const int64 current_pos = input_buffer_->Tell();
            DCHECK_GE(file_pos_limit_, 0);
            if (current_pos < file_pos_limit_) {
              string record;
              TF_RETURN_IF_ERROR(
                  input_buffer_->ReadNBytes(dataset()->record_bytes_, &record));
              // Produce the record as output.
              Tensor record_tensor(ctx->allocator({}), DT_STRING, {});
              record_tensor.scalar<string>()() = record;
              out_tensors->emplace_back(std::move(record_tensor));
              *end_of_sequence = false;
              return Status::OK();
            }

            // We have reached the end of the current file, so maybe
            // move on to next file.
            input_buffer_.reset();
            file_.reset();
            ++current_file_index_;
          }

          // Iteration ends when there are no more files to process.
          if (current_file_index_ == dataset()->filenames_.size()) {
            *end_of_sequence = true;
            return Status::OK();
          }

          // Actually move on to next file.
          uint64 file_size;
          TF_RETURN_IF_ERROR(ctx->env()->GetFileSize(
              dataset()->filenames_[current_file_index_], &file_size));
          file_pos_limit_ = file_size - dataset()->footer_bytes_;

          uint64 body_size =
              file_size - (dataset()->header_bytes_ + dataset()->footer_bytes_);

          if (body_size % dataset()->record_bytes_ != 0) {
            return errors::InvalidArgument(
                "Excluding the header (", dataset()->header_bytes_,
                " bytes) and footer (", dataset()->footer_bytes_,
                " bytes), input file \"",
                dataset()->filenames_[current_file_index_],
                "\" has body length ", body_size,
                " bytes, which is not an exact multiple of the record length (",
                dataset()->record_bytes_, " bytes).");
          }
          TF_RETURN_IF_ERROR(ctx->env()->NewRandomAccessFile(
              dataset()->filenames_[current_file_index_], &file_));
          input_buffer_.reset(
              new io::InputBuffer(file_.get(), dataset()->buffer_size_));
          TF_RETURN_IF_ERROR(
              input_buffer_->SkipNBytes(dataset()->header_bytes_));
        } while (true);
      }

     protected:
      Status SaveInternal(IteratorStateWriter* writer) override {
        mutex_lock l(mu_);
        TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("current_file_index"),
                                               current_file_index_));

        // `input_buffer_` is empty if
        // 1. GetNext has not been called even once.
        // 2. All files have been read and iterator has been exhausted.
        int64 current_pos = input_buffer_ ? input_buffer_->Tell() : -1;
        TF_RETURN_IF_ERROR(
            writer->WriteScalar(full_name("current_pos"), current_pos));
        return Status::OK();
      }

      Status RestoreInternal(IteratorContext* ctx,
                             IteratorStateReader* reader) override {
        mutex_lock l(mu_);
        int64 current_file_index;
        TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("current_file_index"),
                                              &current_file_index));
        current_file_index_ = size_t(current_file_index);
        int64 current_pos;
        TF_RETURN_IF_ERROR(
            reader->ReadScalar(full_name("current_pos"), &current_pos));

        // Seek to current_pos.
        input_buffer_.reset();
        file_.reset();
        if (current_pos >= 0) {  // There was an active input_buffer_.
          uint64 file_size;
          TF_RETURN_IF_ERROR(ctx->env()->GetFileSize(
              dataset()->filenames_[current_file_index_], &file_size));
          file_pos_limit_ = file_size - dataset()->footer_bytes_;
          TF_RETURN_IF_ERROR(ctx->env()->NewRandomAccessFile(
              dataset()->filenames_[current_file_index_], &file_));
          input_buffer_.reset(
              new io::InputBuffer(file_.get(), dataset()->buffer_size_));
          TF_RETURN_IF_ERROR(input_buffer_->Seek(current_pos));
        }

        return Status::OK();
      }

     private:
      mutex mu_;
      size_t current_file_index_ GUARDED_BY(mu_) = 0;
      std::unique_ptr<RandomAccessFile> file_
          GUARDED_BY(mu_);  // must outlive input_buffer_
      std::unique_ptr<io::InputBuffer> input_buffer_ GUARDED_BY(mu_);
      int64 file_pos_limit_ GUARDED_BY(mu_) = -1;
    };

    const std::vector<string> filenames_;
    const int64 header_bytes_;
    const int64 record_bytes_;
    const int64 footer_bytes_;
    const int64 buffer_size_;
  };
};

REGISTER_KERNEL_BUILDER(Name("FixedLengthRecordDataset").Device(DEVICE_CPU),
                        FixedLengthRecordDatasetOp);

class TFRecordDatasetOp : public DatasetOpKernel {
 public:
  using DatasetOpKernel::DatasetOpKernel;

  void MakeDataset(OpKernelContext* ctx, DatasetBase** output) override {
    const Tensor* filenames_tensor;
    OP_REQUIRES_OK(ctx, ctx->input("filenames", &filenames_tensor));
    OP_REQUIRES(
        ctx, filenames_tensor->dims() <= 1,
        errors::InvalidArgument("`filenames` must be a scalar or a vector."));

    std::vector<string> filenames;
    filenames.reserve(filenames_tensor->NumElements());
    for (int i = 0; i < filenames_tensor->NumElements(); ++i) {
      filenames.push_back(filenames_tensor->flat<string>()(i));
    }

    string compression_type;
    OP_REQUIRES_OK(ctx, ParseScalarArgument<string>(ctx, "compression_type",
                                                    &compression_type));

    int64 buffer_size = -1;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "buffer_size", &buffer_size));
    OP_REQUIRES(ctx, buffer_size >= 0,
                errors::InvalidArgument(
                    "`buffer_size` must be >= 0 (0 == no buffering)"));

    *output =
        new Dataset(ctx, std::move(filenames), compression_type, buffer_size);
  }

 private:
  class Dataset : public GraphDatasetBase {
   public:
    explicit Dataset(OpKernelContext* ctx, std::vector<string> filenames,
                     const string& compression_type, int64 buffer_size)
        : GraphDatasetBase(ctx),
          filenames_(std::move(filenames)),
          compression_type_(compression_type),
          options_(io::RecordReaderOptions::CreateRecordReaderOptions(
              compression_type)) {
      if (buffer_size > 0) {
        options_.buffer_size = buffer_size;
      }
    }

    std::unique_ptr<IteratorBase> MakeIterator(
        const string& prefix) const override {
      return std::unique_ptr<IteratorBase>(
          new Iterator({this, strings::StrCat(prefix, "::TFRecord")}));
    }

    const DataTypeVector& output_dtypes() const override {
      static DataTypeVector* dtypes = new DataTypeVector({DT_STRING});
      return *dtypes;
    }

    const std::vector<PartialTensorShape>& output_shapes() const override {
      static std::vector<PartialTensorShape>* shapes =
          new std::vector<PartialTensorShape>({{}});
      return *shapes;
    }

    string DebugString() override { return "TFRecordDatasetOp::Dataset"; }

   protected:
    Status AsGraphDefInternal(DatasetGraphDefBuilder* b,
                              Node** output) const override {
      Node* filenames = nullptr;
      TF_RETURN_IF_ERROR(b->AddVector(filenames_, &filenames));
      Node* compression_type = nullptr;
      TF_RETURN_IF_ERROR(b->AddScalar(compression_type_, &compression_type));
      Node* buffer_size = nullptr;
      TF_RETURN_IF_ERROR(b->AddScalar(options_.buffer_size, &buffer_size));
      TF_RETURN_IF_ERROR(b->AddDataset(
          this, {filenames, compression_type, buffer_size}, output));
      return Status::OK();
    }

   private:
    class Iterator : public DatasetIterator<Dataset> {
     public:
      explicit Iterator(const Params& params)
          : DatasetIterator<Dataset>(params) {}

      Status GetNextInternal(IteratorContext* ctx,
                             std::vector<Tensor>* out_tensors,
                             bool* end_of_sequence) override {
        mutex_lock l(mu_);
        do {
          // We are currently processing a file, so try to read the next record.
          if (reader_) {
            Tensor result_tensor(ctx->allocator({}), DT_STRING, {});
            Status s = reader_->ReadRecord(&result_tensor.scalar<string>()());
            if (s.ok()) {
              out_tensors->emplace_back(std::move(result_tensor));
              *end_of_sequence = false;
              return Status::OK();
            } else if (!errors::IsOutOfRange(s)) {
              return s;
            }

            // We have reached the end of the current file, so maybe
            // move on to next file.
            ResetStreamsLocked();
            ++current_file_index_;
          }

          // Iteration ends when there are no more files to process.
          if (current_file_index_ == dataset()->filenames_.size()) {
            *end_of_sequence = true;
            return Status::OK();
          }

          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
        } while (true);
      }

     protected:
      Status SaveInternal(IteratorStateWriter* writer) override {
        mutex_lock l(mu_);
        TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("current_file_index"),
                                               current_file_index_));

        if (reader_) {
          TF_RETURN_IF_ERROR(
              writer->WriteScalar(full_name("offset"), reader_->TellOffset()));
        }
        return Status::OK();
      }

      Status RestoreInternal(IteratorContext* ctx,
                             IteratorStateReader* reader) override {
        mutex_lock l(mu_);
        ResetStreamsLocked();
        int64 current_file_index;
        TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("current_file_index"),
                                              &current_file_index));
        current_file_index_ = size_t(current_file_index);
        if (reader->Contains(full_name("offset"))) {
          int64 offset;
          TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("offset"), &offset));
          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
          TF_RETURN_IF_ERROR(reader_->SeekOffset(offset));
        }
        return Status::OK();
      }

     private:
      // Sets up reader streams to read from the file at `current_file_index_`.
      Status SetupStreamsLocked(Env* env) EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        if (current_file_index_ >= dataset()->filenames_.size()) {
          return errors::InvalidArgument(
              "current_file_index_:", current_file_index_,
              " >= filenames_.size():", dataset()->filenames_.size());
        }

        // Actually move on to next file.
        const string& next_filename =
            dataset()->filenames_[current_file_index_];
        TF_RETURN_IF_ERROR(env->NewRandomAccessFile(next_filename, &file_));
        reader_.reset(
            new io::SequentialRecordReader(file_.get(), dataset()->options_));
        return Status::OK();
      }

      // Resets all reader streams.
      void ResetStreamsLocked() EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        reader_.reset();
        file_.reset();
      }

      mutex mu_;
      size_t current_file_index_ GUARDED_BY(mu_) = 0;

      // `reader_` will borrow the object that `file_` points to, so
      // we must destroy `reader_` before `file_`.
      std::unique_ptr<RandomAccessFile> file_ GUARDED_BY(mu_);
      std::unique_ptr<io::SequentialRecordReader> reader_ GUARDED_BY(mu_);
    };

    const std::vector<string> filenames_;
    const string compression_type_;
    io::RecordReaderOptions options_;
  };
};

REGISTER_KERNEL_BUILDER(Name("TFRecordDataset").Device(DEVICE_CPU),
                        TFRecordDatasetOp);
class ORCFileDatasetOp : public DatasetOpKernel {
 public:
  using DatasetOpKernel::DatasetOpKernel;

  void MakeDataset(OpKernelContext* ctx, DatasetBase** output) override {
    const Tensor* filenames_tensor;
    OP_REQUIRES_OK(ctx, ctx->input("filenames", &filenames_tensor));
    OP_REQUIRES(
        ctx, filenames_tensor->dims() <= 1,
        errors::InvalidArgument("`filenames` must be a scalar or a vector."));

    std::vector<string> filenames;
    filenames.reserve(filenames_tensor->NumElements());
    for (int i = 0; i < filenames_tensor->NumElements(); ++i) {
      filenames.push_back(filenames_tensor->flat<string>()(i));
    }

    string compression_type;
    OP_REQUIRES_OK(ctx, ParseScalarArgument<string>(ctx, "compression_type",
                                                    &compression_type));

    *output = new Dataset(ctx, std::move(filenames), compression_type);
  }

 private:
  class Dataset : public GraphDatasetBase {
   public:
    explicit Dataset(OpKernelContext* ctx, std::vector<string> filenames,
                     const string& compression_type)
        : GraphDatasetBase(ctx),
          filenames_(std::move(filenames)),
          compression_type_(compression_type) {
      params_.set("number.tuples.per.batch", std::to_string(batch_size_));
      params_.set("table.options", "{\"compresstype\":\"" + compression_type_ +
                                       "\",\"rlecoder\":\"v2\","
                                       "\"bloomfilter\":\"0\"}");
      format_ = storage::Format::createFormat(univplan::ORC_FORMAT, &params_);
    }

    std::unique_ptr<IteratorBase> MakeIterator(
        const string& prefix) const override {
      return std::unique_ptr<IteratorBase>(
          new Iterator({this, strings::StrCat(prefix, "::ORCFile")}));
    }

    const DataTypeVector& output_dtypes() const override {
      static DataTypeVector* dtypes = new DataTypeVector({DT_STRING});
      return *dtypes;
    }

    const std::vector<PartialTensorShape>& output_shapes() const override {
      static std::vector<PartialTensorShape>* shapes =
          new std::vector<PartialTensorShape>({{}});
      return *shapes;
    }

    string DebugString() override { return "ORCFileDatasetOp::Dataset"; }

   protected:
    Status AsGraphDefInternal(DatasetGraphDefBuilder* b,
                              Node** output) const override {
      Node* filenames = nullptr;
      TF_RETURN_IF_ERROR(b->AddVector(filenames_, &filenames));
      Node* compression_type = nullptr;
      TF_RETURN_IF_ERROR(b->AddScalar(compression_type_, &compression_type));
      TF_RETURN_IF_ERROR(
          b->AddDataset(this, {filenames, compression_type}, output));
      return Status::OK();
    }

   private:
    class Iterator : public DatasetIterator<Dataset> {
     public:
      explicit Iterator(const Params& params)
          : DatasetIterator<Dataset>(params) {
        dataset()->format_->setFileSystemManager(&FSManager_);
        offset_.reserve(dataset()->filenames_.size());
        for (size_t i = 0; i < dataset()->filenames_.size(); ++i) {
          offset_.push_back(0);
        }
      }

      Status GetNextInternal(IteratorContext* ctx,
                             std::vector<Tensor>* out_tensors,
                             bool* end_of_sequence) override {
        mutex_lock l(mu_);
        do {
          // We are currently processing a file, so try to read the next record.
          if (file_splits_) {
            Tensor result_tensor(ctx->allocator({}), DT_STRING, {});
            uint64 rowId =
                offset_[current_file_index_] % dataset()->batch_size_;
            if (rowId == 0) {
              result_ = dataset()->format_->next();
            }
            if (result_ != nullptr) {
              Status s = orcExample(rowId, &result_tensor.scalar<string>()());
              if (s.ok()) {
                offset_[current_file_index_]++;
                out_tensors->emplace_back(std::move(result_tensor));
                *end_of_sequence = false;
                return Status::OK();
              } else if (!errors::IsOutOfRange(s)) {
                return s;
              }
            }

            // We have reached the end of the current file, so maybe
            // move on to next file.
            ResetStreamsLocked();
            ++current_file_index_;
          }

          // Iteration ends when there are no more files to process.
          if (current_file_index_ == dataset()->filenames_.size()) {
            *end_of_sequence = true;
            return Status::OK();
          }

          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
        } while (true);
      }

     protected:
      Status SaveInternal(IteratorStateWriter* writer) override {
        mutex_lock l(mu_);
        TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("current_file_index"),
                                               current_file_index_));

        if (file_splits_) {
          TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("offset"),
                                                 offset_[current_file_index_]));
        }
        return Status::OK();
      }

      Status RestoreInternal(IteratorContext* ctx,
                             IteratorStateReader* reader) override {
        mutex_lock l(mu_);
        ResetStreamsLocked();
        int64 current_file_index;
        TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("current_file_index"),
                                              &current_file_index));
        current_file_index_ = size_t(current_file_index);
        if (reader->Contains(full_name("offset"))) {
          int64 offset;
          TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("offset"), &offset));
          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
          offset_[current_file_index_] = offset;
        }
        return Status::OK();
      }

     private:
      // Sets up reader streams to read from the file at `current_file_index_`.
      Status SetupStreamsLocked(Env* env) EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        if (current_file_index_ >= dataset()->filenames_.size()) {
          return errors::InvalidArgument(
              "current_file_index_:", current_file_index_,
              " >= filenames_.size():", dataset()->filenames_.size());
        }

        // Actually move on to next file.
        const string& next_filename =
            dataset()->filenames_[current_file_index_];
        offset_[current_file_index_] = 0;
        string protocol, path;
        getPathFromUrl(next_filename, protocol, path);
        string fullFileName = protocol + path;
        dbcommon::FileSystem* fs = FSManager_.get(fullFileName.c_str());
        if (!fs->exists(path.c_str())) return errors::NotFound(path.c_str());

        std::unique_ptr<dbcommon::FileInfo> info = fs->getFileInfo(path.c_str());
        int64_t len = info->size;
        int64_t start = 0;
        file_splits_.reset(new univplan::UnivPlanScanFileSplitList());
        univplan::UnivPlanScanFileSplit* file_split = file_splits_->Add();
        file_split->set_filename(fullFileName.c_str());
        file_split->set_start(start);
        file_split->set_len(len);
        dbcommon::TupleDesc desc;
        dataset()->format_->beginScan(file_splits_.get(), &desc, nullptr, nullptr,
                                      false);
        return Status::OK();
      }

      // Resets all reader streams.
      void ResetStreamsLocked() EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        dataset()->format_->endScan();
        file_splits_.reset();
        result_.reset();
      }

      void AddInt64Feature(Features* features, string key, int64_t value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        Feature feature;
        feature.mutable_int64_list()->add_value(value);
        auto map = features->mutable_feature();
        (*map)[key] = feature;
      }

      void AddFloatFeature(Features* features, string key, float value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        Feature feature;
        feature.mutable_float_list()->add_value(value);
        auto map = features->mutable_feature();
        (*map)[key] = feature;
      }

      void AddBytesFeature(Features* features, string key, string value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        Feature feature;
        feature.mutable_bytes_list()->add_value(value);
        auto map = features->mutable_feature();
        (*map)[key] = feature;
      }

      Status orcExample(uint64_t rowId, string* value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {  // start function orcExample
        if (result_ != nullptr) {          // start region if
          const dbcommon::TupleBatchReader& reader =
              result_->getTupleBatchReader();
          if (rowId >= result_->getNumOfRows()) {
            return errors::OutOfRange("file to end.");
          }

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
          return Status::OK();
        }  // end region if
      }    // end function orcExample

      void getPathFromUrl(const string& url, string& protocol, string& path)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
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

      mutex mu_;
      size_t current_file_index_ GUARDED_BY(mu_) = 0;

      // `reader_` will borrow the object that `file_` points to, so
      // we must destroy `reader_` before `file_`.
      dbcommon::TupleBatch::uptr result_ GUARDED_BY(mu_);
      std::unique_ptr<univplan::UnivPlanScanFileSplitList> file_splits_ GUARDED_BY(mu_);
      std::vector<uint64> offset_ GUARDED_BY(mu_);
      dbcommon::FileSystemManager FSManager_ GUARDED_BY(mu_);
    };

    const std::vector<string> filenames_;
    const string compression_type_;
    const int batch_size_ = 32;
    dbcommon::Parameters params_;
    std::unique_ptr<storage::Format> format_;
  };
};

REGISTER_KERNEL_BUILDER(Name("ORCFileDataset").Device(DEVICE_CPU),
                        ORCFileDatasetOp);

class OmniFileDatasetOp : public DatasetOpKernel {
 public:
  using DatasetOpKernel::DatasetOpKernel;

  void MakeDataset(OpKernelContext* ctx, DatasetBase** output) override {
    const Tensor* filenames_tensor;
    OP_REQUIRES_OK(ctx, ctx->input("filenames", &filenames_tensor));
    OP_REQUIRES(
        ctx, filenames_tensor->dims() <= 1,
        errors::InvalidArgument("`filenames` must be a scalar or a vector."));

    std::vector<string> filenames;
    filenames.reserve(filenames_tensor->NumElements());
    for (int i = 0; i < filenames_tensor->NumElements(); ++i) {
      filenames.push_back(filenames_tensor->flat<string>()(i));
    }

    int64 data_format_type = 1;
    univplan::UNIVPLANFORMATTYPE format_tpye;
    OP_REQUIRES_OK(ctx, ParseScalarArgument<int64>(ctx, "data_format_type",
                                                   &data_format_type));
    OP_REQUIRES(
        ctx, data_format_type == 1,
        errors::InvalidArgument("data_format_type only support orc(1) yet",
                                data_format_type));
    switch (data_format_type) {
      case 1:
        format_tpye = univplan::UNIVPLANFORMATTYPE::ORC_FORMAT;
        break;
    }

    string compression_type = "snappy";
    OP_REQUIRES_OK(ctx, ParseScalarArgument<string>(ctx, "compression_type",
                                                    &compression_type));

    int64 block_count = 1;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "block_count", &block_count));
    OP_REQUIRES(ctx, block_count >= 1,
                errors::InvalidArgument("block_count must be >=1 not",
                                        data_format_type));

    int64 block_index = 0;
    OP_REQUIRES_OK(
        ctx, ParseScalarArgument<int64>(ctx, "block_index", &block_index));
    OP_REQUIRES(
        ctx, block_index >= 0 && block_index < block_count,
        errors::InvalidArgument("block_index must be >=0 and < block_count not",
                                block_index));

    *output = new Dataset(ctx, std::move(filenames), format_tpye,
                          compression_type, block_count, block_index);
  }

 private:
  class Dataset : public GraphDatasetBase {
   public:
    explicit Dataset(OpKernelContext* ctx, std::vector<string> filenames,
                     univplan::UNIVPLANFORMATTYPE data_format_type,
                     const string& compression_type, int block_count,
                     int block_index)
        : GraphDatasetBase(ctx),
          filenames_(std::move(filenames)),
          data_format_type_(data_format_type),
          compression_type_(compression_type),
          block_count_(block_count),
          block_index_(block_index) {
      params_.set("number.tuples.per.batch", std::to_string(batch_size_));
      params_.set("table.options", "{\"compresstype\":\"" + compression_type_ +
                                       "\",\"rlecoder\":\"v2\","
                                       "\"bloomfilter\":\"0\"}");
      format_ = storage::Format::createFormat(data_format_type_, &params_);
    }

    std::unique_ptr<IteratorBase> MakeIterator(
        const string& prefix) const override {
      return std::unique_ptr<IteratorBase>(
          new Iterator({this, strings::StrCat(prefix, "::OmniFile")}));
    }

    const DataTypeVector& output_dtypes() const override {
      static DataTypeVector* dtypes = new DataTypeVector({DT_STRING});
      return *dtypes;
    }

    const std::vector<PartialTensorShape>& output_shapes() const override {
      static std::vector<PartialTensorShape>* shapes =
          new std::vector<PartialTensorShape>({{}});
      return *shapes;
    }

    string DebugString() override { return "OmniFileDatasetOp::Dataset"; }

   protected:
    Status AsGraphDefInternal(DatasetGraphDefBuilder* b,
                              Node** output) const override {
       Node* filenames = nullptr;
      TF_RETURN_IF_ERROR(b->AddVector(filenames_, &filenames));
      Node* data_format_type = nullptr;
      TF_RETURN_IF_ERROR(b->AddScalar(data_format_type_, &data_format_type));
      Node* compression_type = nullptr;
      TF_RETURN_IF_ERROR(b->AddScalar(compression_type_, &compression_type));
      Node* block_count = nullptr;
      TF_RETURN_IF_ERROR(b->AddScalar(block_count_, &block_count));
      Node* block_index = nullptr;
      TF_RETURN_IF_ERROR(b->AddScalar(block_index_, &block_index));
      TF_RETURN_IF_ERROR(
          b->AddDataset(this, {filenames, data_format_type, compression_type, block_count, block_index}, output));
      return Status::OK();
    }

   private:
    class Iterator : public DatasetIterator<Dataset> {
     public:
      explicit Iterator(const Params& params)
          : DatasetIterator<Dataset>(params) {
        dataset()->format_->setFileSystemManager(&FSManager_);
        offset_.reserve(dataset()->filenames_.size());
        for (size_t i = 0; i < dataset()->filenames_.size(); ++i) {
          offset_.push_back(0);
        }
      }

      Status GetNextInternal(IteratorContext* ctx,
                             std::vector<Tensor>* out_tensors,
                             bool* end_of_sequence) override {
        mutex_lock l(mu_);
        do {
          // We are currently processing a file, so try to read the next record.
          if (file_splits_) {
            Tensor result_tensor(ctx->allocator({}), DT_STRING, {});
            uint64 rowId =
                offset_[current_file_index_] % dataset()->batch_size_;
            if (rowId == 0) {
              result_ = dataset()->format_->next();
            }
            if (result_ != nullptr) {
              Status s =
                  DataRowExample(rowId, &result_tensor.scalar<string>()());
              if (s.ok()) {
                offset_[current_file_index_]++;
                out_tensors->emplace_back(std::move(result_tensor));
                *end_of_sequence = false;
                return Status::OK();
              } else if (!errors::IsOutOfRange(s)) {
                return s;
              }
            }

            // We have reached the end of the current file, so maybe
            // move on to next file.
            ResetStreamsLocked();
            ++current_file_index_;
          }

          // Iteration ends when there are no more files to process.
          if (current_file_index_ == dataset()->filenames_.size()) {
            *end_of_sequence = true;
            return Status::OK();
          }

          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
        } while (true);
      }

     protected:
      Status SaveInternal(IteratorStateWriter* writer) override {
        mutex_lock l(mu_);
        TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("current_file_index"),
                                               current_file_index_));
        // write block count and index
        TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("block_count"),
                                               dataset()->block_count_));
        TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("block_index"),
                                               dataset()->block_index_));
        if (file_splits_) {
          TF_RETURN_IF_ERROR(writer->WriteScalar(full_name("offset"),
                                                 offset_[current_file_index_]));
        }
        return Status::OK();
      }

      Status RestoreInternal(IteratorContext* ctx,
                             IteratorStateReader* reader) override {
        mutex_lock l(mu_);
        ResetStreamsLocked();
        int64 current_file_index;
        TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("current_file_index"),
                                              &current_file_index));
        current_file_index_ = size_t(current_file_index);
        if (reader->Contains(full_name("offset"))) {
          int64 offset;
          TF_RETURN_IF_ERROR(reader->ReadScalar(full_name("offset"), &offset));
          TF_RETURN_IF_ERROR(SetupStreamsLocked(ctx->env()));
          offset_[current_file_index_] = offset;
          auto batch_count = offset / dataset()->batch_size_;
          for (size_t i = 0; i < batch_count; i++) {
            dataset()->format_->next();
          }
        }
        return Status::OK();
      }

     private:
      // Sets up reader streams to read from the file at `current_file_index_`.
      Status SetupStreamsLocked(Env* env) EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        if (current_file_index_ >= dataset()->filenames_.size()) {
          return errors::InvalidArgument(
              "current_file_index_:", current_file_index_,
              " >= filenames_.size():", dataset()->filenames_.size());
        }

        // Actually move on to next file.
        const string& next_filename =
            dataset()->filenames_[current_file_index_];
        offset_[current_file_index_] = 0;
        string protocol, path;
        getPathFromUrl(next_filename, protocol, path);
        string fullFileName = protocol + path;
        dbcommon::FileSystem* fs = FSManager_.get(fullFileName.c_str());
        if (!fs->exists(path.c_str())) return errors::NotFound(path.c_str());

        std::unique_ptr<dbcommon::FileInfo> info = fs->getFileInfo(path.c_str());
        int64_t len = info->size / dataset()->block_count_;
        int64_t start = dataset()->block_index_ * len;
        if (dataset()->block_index_ == dataset()->block_count_ - 1) {
          len = info->size - start;
        }
        file_splits_.reset(new univplan::UnivPlanScanFileSplitList());
        univplan::UnivPlanScanFileSplit* file_split = file_splits_->Add();
        file_split->set_filename(info->name.c_str());
        file_split->set_start(start);
        file_split->set_len(len);
        dataset()->format_->beginScan(file_splits_.get(), nullptr, nullptr, nullptr,
                                      false);
        return Status::OK();
      }

      // Resets all reader streams.
      void ResetStreamsLocked() EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        dataset()->format_->endScan();
        file_splits_.reset();
        result_.reset();
      }

      void AddInt64Feature(Features* features, string key, int64_t value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        Feature feature;
        feature.mutable_int64_list()->add_value(value);
        auto map = features->mutable_feature();
        (*map)[key] = feature;
      }

      void AddFloatFeature(Features* features, string key, float value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        Feature feature;
        feature.mutable_float_list()->add_value(value);
        auto map = features->mutable_feature();
        (*map)[key] = feature;
      }

      void AddBytesFeature(Features* features, string key, string value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
        Feature feature;
        feature.mutable_bytes_list()->add_value(value);
        auto map = features->mutable_feature();
        (*map)[key] = feature;
      }

      Status DataRowExample(uint64_t rowId, string* value)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {  // start function orcExample
        if (result_ != nullptr) {          // start region if
          const dbcommon::TupleBatchReader& reader =
              result_->getTupleBatchReader();
          if (rowId >= result_->getNumOfRows()) {
            return errors::OutOfRange("file to end.");
          }

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
          return Status::OK();
        }  // end region if
      }    // end function DataRowExample

      void getPathFromUrl(const string& url, string& protocol, string& path)
          EXCLUSIVE_LOCKS_REQUIRED(mu_) {
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

      mutex mu_;
      size_t current_file_index_ GUARDED_BY(mu_) = 0;

      // `reader_` will borrow the object that `file_` points to, so
      // we must destroy `reader_` before `file_`.
      dbcommon::TupleBatch::uptr result_ GUARDED_BY(mu_);
      std::unique_ptr<univplan::UnivPlanScanFileSplitList> file_splits_ GUARDED_BY(mu_);
      std::vector<uint64> offset_ GUARDED_BY(mu_);
      dbcommon::FileSystemManager FSManager_ GUARDED_BY(mu_);
    };

    const std::vector<string> filenames_;
    const string compression_type_;
    const univplan::UNIVPLANFORMATTYPE data_format_type_;
    const int batch_size_ = 32;
    int64 block_count_ = 1;
    int64 block_index_ = 0;
    dbcommon::Parameters params_;
    std::unique_ptr<storage::Format> format_;
  };
};

REGISTER_KERNEL_BUILDER(Name("OmniFileDataset").Device(DEVICE_CPU),
                        OmniFileDatasetOp);
}  // namespace

}  // namespace tensorflow

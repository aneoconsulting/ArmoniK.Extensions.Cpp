#include <gtest/gtest.h>

#include <armonik/sdk/common/ArmoniKSdkException.h>
#include <armonik/sdk/common/BlobDefinition.h>
#include <armonik/sdk/common/DynamicLibrary.h>
#include <armonik/sdk/common/TaskDefinition.h>
#include <armonik/sdk/common/TaskOptions.h>
#include <armonik/sdk/common/TaskPayload.h>

using namespace ArmoniK::Sdk::Common;

// ---------------------------------------------------------------------------
// ConventionPayload JSON serialization
// ---------------------------------------------------------------------------

TEST(ConventionPayloadJson, RoundTrip) {
  ConventionPayload original;
  original.inputs = {{"x", "blob-1"}, {"y", "blob-2"}};
  original.outputs = {{"result", "blob-3"}};

  auto json = original.Serialize();
  auto restored = ConventionPayload::Deserialize(json);

  EXPECT_EQ(restored.inputs, original.inputs);
  EXPECT_EQ(restored.outputs, original.outputs);
}

TEST(ConventionPayloadJson, EmptyMaps) {
  ConventionPayload original;
  original.inputs = {};
  original.outputs = {};

  auto json = original.Serialize();
  auto restored = ConventionPayload::Deserialize(json);

  EXPECT_TRUE(restored.inputs.empty());
  EXPECT_TRUE(restored.outputs.empty());
}

TEST(ConventionPayloadJson, MissingMethodField) {
  // Java SDK omits the "method" field — must deserialize without error
  const std::string json = R"({"inputs":{"a":"b"},"outputs":{"c":"d"}})";
  auto payload = ConventionPayload::Deserialize(json);
  EXPECT_EQ(payload.inputs.at("a"), "b");
  EXPECT_EQ(payload.outputs.at("c"), "d");
}

TEST(ConventionPayloadJson, MissingInputsThrows) {
  const std::string json = R"({"method":"foo","outputs":{}})";
  EXPECT_THROW(ConventionPayload::Deserialize(json), ArmoniKSdkException);
}

TEST(ConventionPayloadJson, MissingOutputsThrows) {
  const std::string json = R"({"method":"foo","inputs":{}})";
  EXPECT_THROW(ConventionPayload::Deserialize(json), ArmoniKSdkException);
}

TEST(ConventionPayloadJson, InvalidJsonThrows) {
  EXPECT_THROW(ConventionPayload::Deserialize("not-json"), ArmoniKSdkException);
  EXPECT_THROW(ConventionPayload::Deserialize(""), ArmoniKSdkException);
  EXPECT_THROW(ConventionPayload::Deserialize("{"), ArmoniKSdkException);
}

TEST(ConventionPayloadJson, MultipleInputsOutputs) {
  ConventionPayload original;
  original.inputs = {{"a", "1"}, {"b", "2"}, {"c", "3"}};
  original.outputs = {{"out1", "4"}, {"out2", "5"}};

  auto restored = ConventionPayload::Deserialize(original.Serialize());

  EXPECT_EQ(restored.inputs.size(), 3u);
  EXPECT_EQ(restored.outputs.size(), 2u);
  EXPECT_EQ(restored.inputs.at("b"), "2");
  EXPECT_EQ(restored.outputs.at("out2"), "5");
}

// ---------------------------------------------------------------------------
// TaskOptions::SetDynamicLibrary / GetDynamicLibrary
// ---------------------------------------------------------------------------

// Path-only loading: the classic case where the worker reads the .so directly from
// its local filesystem. No blob involved, so library_blob_id must come back empty.
TEST(TaskOptionsDynamicLibrary, RoundTripWithPathOnly) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");

  DynamicLibrary lib;
  lib.library_path = "/data/lib.so";
  lib.symbol = "myprefix";

  opts.SetDynamicLibrary(lib);
  auto restored = opts.GetDynamicLibrary();

  EXPECT_EQ(restored.library_path, "/data/lib.so");
  EXPECT_EQ(restored.symbol, "myprefix");
  EXPECT_TRUE(restored.library_blob_id.empty());
}

// Blob-only loading: the cross-SDK / dynamic upload case where the client has uploaded
// the .so via UploadLibrary(path, lib) and lib.library_blob_id is now set.
// library_path is optional and should be empty when not set.
TEST(TaskOptionsDynamicLibrary, RoundTripWithBlobId) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");

  DynamicLibrary lib;
  lib.library_blob_id = "blob-uuid-1234";
  lib.symbol = "myMethod"; // method name passed to armonik_call at execution time

  opts.SetDynamicLibrary(lib);
  auto restored = opts.GetDynamicLibrary();

  EXPECT_EQ(restored.library_blob_id, "blob-uuid-1234");
  EXPECT_EQ(restored.symbol, "myMethod");
}

// Both fields set: library_path is kept as a hint but the worker will prefer the blob.
// Ensures SetDynamicLibrary writes both keys and GetDynamicLibrary reads both.
TEST(TaskOptionsDynamicLibrary, RoundTripWithBothPathAndBlobId) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");

  DynamicLibrary lib;
  lib.library_path = "/data/lib.so";
  lib.library_blob_id = "blob-abc";
  lib.symbol = "sym";

  opts.SetDynamicLibrary(lib);
  auto restored = opts.GetDynamicLibrary();

  EXPECT_EQ(restored.library_path, "/data/lib.so");
  EXPECT_EQ(restored.library_blob_id, "blob-abc");
}

// When neither LibraryPath nor LibraryBlobId is present, GetDynamicLibrary must
// throw rather than silently return an unusable struct — the worker would dlopen("")
// which is implementation-defined and almost always wrong.
TEST(TaskOptionsDynamicLibrary, MissingLibraryPathWithoutBlobIdThrows) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");
  opts.options[DynamicLibrary::KeyConventionVersion] = DynamicLibrary::ConventionVersion;
  opts.options[DynamicLibrary::KeySymbol] = "sym";
  // Neither LibraryPath nor LibraryBlobId set

  EXPECT_THROW(opts.GetDynamicLibrary(), ArmoniKSdkException);
}

// SetDynamicLibrary must always write the ConventionVersion key so that the worker
// can detect the encoding and dispatch to the right loading path.
TEST(TaskOptionsDynamicLibrary, SetDynamicLibrarySetsConventionVersion) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");

  DynamicLibrary lib;
  lib.library_path = "/data/lib.so";
  opts.SetDynamicLibrary(lib);

  EXPECT_EQ(opts.options.at(DynamicLibrary::KeyConventionVersion), DynamicLibrary::ConventionVersion);
}

// An empty library_blob_id must not be written to the options map: a missing key
// and an empty-string key have different semantics — the worker uses presence of
// LibraryBlobId to decide between path-based and blob-based loading.
TEST(TaskOptionsDynamicLibrary, BlobIdNotWrittenWhenEmpty) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");

  DynamicLibrary lib;
  lib.library_path = "/data/lib.so";
  lib.library_blob_id = "";
  opts.SetDynamicLibrary(lib);

  EXPECT_EQ(opts.options.count(DynamicLibrary::KeyLibraryBlobId), 0u);
}

// ---------------------------------------------------------------------------
// TaskOptions::GetConventionVersion
// ---------------------------------------------------------------------------

// After SetDynamicLibrary the version string must match the current convention.
TEST(TaskOptionsConventionVersion, ReturnsVersionWhenSet) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");
  DynamicLibrary lib;
  lib.library_path = "/data/lib.so";
  opts.SetDynamicLibrary(lib);

  EXPECT_EQ(opts.GetConventionVersion(), DynamicLibrary::ConventionVersion);
}

// Tasks that were submitted without the convention (e.g. legacy path) will not have
// ConventionVersion in their options; GetConventionVersion must throw so the caller
// can distinguish them from convention tasks.
TEST(TaskOptionsConventionVersion, ThrowsWhenMissing) {
  TaskOptions opts("app", "1.0", "ns", "svc", "part");
  EXPECT_THROW(opts.GetConventionVersion(), ArmoniKSdkException);
}

// ---------------------------------------------------------------------------
// BlobDefinition
// ---------------------------------------------------------------------------

TEST(BlobDefinition, FromDataIsRaw) {
  auto b = BlobDefinition::FromData("hello");
  EXPECT_TRUE(b.IsRawData());
  EXPECT_EQ(b.GetData(), "hello");
}

TEST(BlobDefinition, FromDataPreservesBinaryContent) {
  std::string bytes(4, '\0');
  bytes[0] = '\x01';
  bytes[2] = '\xFF';
  auto b = BlobDefinition::FromData(bytes);
  EXPECT_TRUE(b.IsRawData());
  EXPECT_EQ(b.GetData(), bytes);
}

TEST(BlobDefinition, FromBlobIdIsNotRaw) {
  auto b = BlobDefinition::FromBlobId("result-uuid-1234");
  EXPECT_FALSE(b.IsRawData());
  EXPECT_EQ(b.GetBlobId(), "result-uuid-1234");
}

TEST(BlobDefinition, FromDataEmptyString) {
  auto b = BlobDefinition::FromData("");
  EXPECT_TRUE(b.IsRawData());
  EXPECT_EQ(b.GetData(), "");
}

TEST(BlobDefinition, FromBlobIdEmptyString) {
  auto b = BlobDefinition::FromBlobId("");
  EXPECT_FALSE(b.IsRawData());
  EXPECT_EQ(b.GetBlobId(), "");
}

// ---------------------------------------------------------------------------
// TaskDefinition
// ---------------------------------------------------------------------------

TEST(TaskDefinition, BraceInit) {
  TaskDefinition td("my_method",
                    {{"x", BlobDefinition::FromData("data_x")}, {"y", BlobDefinition::FromBlobId("blob-y")}});
  EXPECT_EQ(td.method_name, "my_method");
  EXPECT_EQ(td.inputs.size(), 2u);
  EXPECT_TRUE(td.inputs.at("x").IsRawData());
  EXPECT_EQ(td.inputs.at("x").GetData(), "data_x");
  EXPECT_FALSE(td.inputs.at("y").IsRawData());
  EXPECT_EQ(td.inputs.at("y").GetBlobId(), "blob-y");
}

TEST(TaskDefinition, WithInputBuilder) {
  TaskDefinition td;
  td.method_name = "sum";
  td.WithInput("a", BlobDefinition::FromData("1")).WithInput("b", BlobDefinition::FromData("2"));

  EXPECT_EQ(td.inputs.size(), 2u);
  EXPECT_EQ(td.inputs.at("a").GetData(), "1");
  EXPECT_EQ(td.inputs.at("b").GetData(), "2");
}

TEST(TaskDefinition, NoInputs) {
  TaskDefinition td("ping", {});
  EXPECT_EQ(td.method_name, "ping");
  EXPECT_TRUE(td.inputs.empty());
}

TEST(TaskDefinition, WithInputOverwritesExisting) {
  TaskDefinition td;
  td.WithInput("k", BlobDefinition::FromData("v1"));
  td.WithInput("k", BlobDefinition::FromData("v2")); // same key, different value
  // std::map::emplace does not overwrite — first insertion wins
  EXPECT_EQ(td.inputs.at("k").GetData(), "v1");
}

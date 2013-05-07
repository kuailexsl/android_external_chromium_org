// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "chrome/browser/sync_file_system/local_sync_operation_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/fileapi/syncable/file_change.h"
#include "webkit/fileapi/syncable/sync_file_type.h"

namespace sync_file_system {

namespace {

struct Input {
  scoped_ptr<FileChange> remote_file_change;
  SyncFileType remote_file_type_in_metadata;

  std::string DebugString() const {
    std::string change_type =
        (remote_file_change == NULL) ? "none"
                                     : remote_file_change->DebugString();
    std::ostringstream ss;
    ss << "RemoteFileChange: " << change_type
       << ", RemoteFileTypeInMetadata: " << remote_file_type_in_metadata;
    return ss.str();
  }

  Input(FileChange* remote_file_change,
        SyncFileType remote_file_type_in_metadata)
      : remote_file_change(remote_file_change),
        remote_file_type_in_metadata(remote_file_type_in_metadata) {
  }
};

template <typename type, size_t array_size>
std::vector<type> CreateList(const type (&inputs)[array_size]) {
  return std::vector<type>(inputs, inputs + array_size);
}

ScopedVector<Input> CreateInput() {
  SyncFileType dummy_file_type = SYNC_FILE_TYPE_UNKNOWN;

  ScopedVector<Input> vector;
  vector.push_back(new Input(NULL, SYNC_FILE_TYPE_UNKNOWN));
  vector.push_back(new Input(NULL, SYNC_FILE_TYPE_FILE));
  vector.push_back(new Input(NULL, SYNC_FILE_TYPE_DIRECTORY));

  // When remote_file_change exists, the resolver does not take care of
  // remote_file_type_in_metadata.
  vector.push_back(new Input(
      new FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                     SYNC_FILE_TYPE_FILE),
      dummy_file_type));
  vector.push_back(new Input(
      new FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                     SYNC_FILE_TYPE_DIRECTORY),
      dummy_file_type));
  vector.push_back(new Input(
      new FileChange(FileChange::FILE_CHANGE_DELETE,
                     SYNC_FILE_TYPE_FILE),
      dummy_file_type));
  vector.push_back(new Input(
      new FileChange(FileChange::FILE_CHANGE_DELETE,
                     SYNC_FILE_TYPE_DIRECTORY),
      dummy_file_type));

  return vector.Pass();
}

std::string DebugString(const ScopedVector<Input>& inputs, int number) {
  std::ostringstream ss;
  ss << "Case " << number << ": (" << inputs[number]->DebugString() << ")";
  return ss.str();
}

}  // namespace

class LocalSyncOperationResolverTest : public testing::Test {
 public:
  LocalSyncOperationResolverTest() {}

 protected:
  typedef LocalSyncOperationResolver Resolver;
  typedef std::vector<LocalSyncOperationType> ExpectedTypes;

  DISALLOW_COPY_AND_ASSIGN(LocalSyncOperationResolverTest);
};

TEST_F(LocalSyncOperationResolverTest, ResolveForAddOrUpdateFile) {
  const LocalSyncOperationType kExpectedTypes[] = {
    LOCAL_SYNC_OPERATION_ADD_FILE,
    LOCAL_SYNC_OPERATION_UPDATE_FILE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,

    LOCAL_SYNC_OPERATION_CONFLICT,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
  };

  ExpectedTypes expected_types = CreateList(kExpectedTypes);
  ScopedVector<Input> inputs = CreateInput();

  ASSERT_EQ(expected_types.size(), inputs.size());
  for (ExpectedTypes::size_type i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(expected_types[i],
              Resolver::ResolveForAddOrUpdateFile(
                  inputs[i]->remote_file_change.get(),
                  inputs[i]->remote_file_type_in_metadata))
        << DebugString(inputs, i);
  }
}

TEST_F(LocalSyncOperationResolverTest, ResolveForAddOrUpdateFileInConflict) {
  const LocalSyncOperationType kExpectedTypes[] = {
    LOCAL_SYNC_OPERATION_CONFLICT,
    LOCAL_SYNC_OPERATION_CONFLICT,
    LOCAL_SYNC_OPERATION_CONFLICT,

    LOCAL_SYNC_OPERATION_CONFLICT,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
  };

  ExpectedTypes expected_types = CreateList(kExpectedTypes);
  ScopedVector<Input> inputs = CreateInput();

  ASSERT_EQ(expected_types.size(), inputs.size());
  for (ExpectedTypes::size_type i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(expected_types[i],
              Resolver::ResolveForAddOrUpdateFileInConflict(
                  inputs[i]->remote_file_change.get()))
        << DebugString(inputs, i);
  }
}

TEST_F(LocalSyncOperationResolverTest, ResolveForAddDirectory) {
  const LocalSyncOperationType kExpectedTypes[] = {
    LOCAL_SYNC_OPERATION_ADD_DIRECTORY,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
    LOCAL_SYNC_OPERATION_NONE,

    LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
    LOCAL_SYNC_OPERATION_NONE,
    LOCAL_SYNC_OPERATION_ADD_DIRECTORY,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
  };

  ExpectedTypes expected_types = CreateList(kExpectedTypes);
  ScopedVector<Input> inputs = CreateInput();

  ASSERT_EQ(expected_types.size(), inputs.size());
  for (ExpectedTypes::size_type i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(expected_types[i],
              Resolver::ResolveForAddDirectory(
                  inputs[i]->remote_file_change.get(),
                  inputs[i]->remote_file_type_in_metadata))
        << DebugString(inputs, i);
  }
}

TEST_F(LocalSyncOperationResolverTest, ResolveForAddDirectoryInConflict) {
  EXPECT_EQ(LOCAL_SYNC_OPERATION_RESOLVE_TO_LOCAL,
            Resolver::ResolveForAddDirectoryInConflict());
}

TEST_F(LocalSyncOperationResolverTest, ResolveForDeleteFile) {
  const LocalSyncOperationType kExpectedTypes[] = {
    LOCAL_SYNC_OPERATION_NONE,
    LOCAL_SYNC_OPERATION_DELETE_FILE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,

    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
  };

  ExpectedTypes expected_types = CreateList(kExpectedTypes);
  ScopedVector<Input> inputs = CreateInput();

  ASSERT_EQ(expected_types.size(), inputs.size());
  for (ExpectedTypes::size_type i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(expected_types[i],
              Resolver::ResolveForDeleteFile(
                  inputs[i]->remote_file_change.get(),
                  inputs[i]->remote_file_type_in_metadata))
        << DebugString(inputs, i);
  }
}

TEST_F(LocalSyncOperationResolverTest, ResolveForDeleteFileInConflict) {
  const LocalSyncOperationType kExpectedTypes[] = {
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,

    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
  };

  ExpectedTypes expected_types = CreateList(kExpectedTypes);
  ScopedVector<Input> inputs = CreateInput();

  ASSERT_EQ(expected_types.size(), inputs.size());
  for (ExpectedTypes::size_type i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(expected_types[i],
              Resolver::ResolveForDeleteFileInConflict(
                  inputs[i]->remote_file_change.get()))
        << DebugString(inputs, i);
  }
}

TEST_F(LocalSyncOperationResolverTest, ResolveForDeleteDirectory) {
  const LocalSyncOperationType kExpectedTypes[] = {
    LOCAL_SYNC_OPERATION_NONE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_DELETE_DIRECTORY,

    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
  };

  ExpectedTypes expected_types = CreateList(kExpectedTypes);
  ScopedVector<Input> inputs = CreateInput();

  ASSERT_EQ(expected_types.size(), inputs.size());
  for (ExpectedTypes::size_type i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(expected_types[i],
              Resolver::ResolveForDeleteDirectory(
                  inputs[i]->remote_file_change.get(),
                  inputs[i]->remote_file_type_in_metadata))
        << DebugString(inputs, i);
  }
}

TEST_F(LocalSyncOperationResolverTest, ResolveForDeleteDirectoryInConflict) {
  const LocalSyncOperationType kExpectedTypes[] = {
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,

    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_RESOLVE_TO_REMOTE,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
    LOCAL_SYNC_OPERATION_DELETE_METADATA,
  };

  ExpectedTypes expected_types = CreateList(kExpectedTypes);
  ScopedVector<Input> inputs = CreateInput();

  ASSERT_EQ(expected_types.size(), inputs.size());
  for (ExpectedTypes::size_type i = 0; i < expected_types.size(); ++i) {
    EXPECT_EQ(expected_types[i],
              Resolver::ResolveForDeleteDirectoryInConflict(
                  inputs[i]->remote_file_change.get()))
        << DebugString(inputs, i);
  }
}

}  // namespace sync_file_system

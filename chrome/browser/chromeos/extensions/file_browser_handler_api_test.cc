// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// File contains browser tests for the fileBrowserHandler api.

#include "chrome/browser/chromeos/extensions/file_browser_handler_api.h"

#include <vector>

#include "base/bind.h"
#include "base/path_service.h"
#include "base/scoped_temp_dir.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_mount_point_provider.h"

namespace utils = extension_function_test_utils;

using content::BrowserContext;
using extensions::Extension;

namespace {

// Data that defines FileSelector behaviour in each test case.
struct TestCase {
  TestCase(const FilePath& suggested_name,
           const std::vector<std::string>& allowed_extensions,
           bool success,
           const FilePath& selected_path)
      : suggested_name(suggested_name),
        allowed_extensions(allowed_extensions),
        success(success),
        selected_path(selected_path) {
  }
  ~TestCase() {}

  // Path that we expect to be suggested to the file selector.
  FilePath suggested_name;

  // Extensions that we expect to be allowed to the file selector.
  std::vector<std::string> allowed_extensions;

  // Whether file selector should fail.
  bool success;
  // The path file selector should return back to the function.
  FilePath selected_path;
};

// Checks that file under path |selected_path| contains |expected_contents|.
// Must be called on the file thread.
void ExpectFileContentEquals(const FilePath& selected_path,
                             const std::string& expected_contents) {
  std::string test_file_contents;
  ASSERT_TRUE(file_util::ReadFileToString(selected_path, &test_file_contents));
  EXPECT_EQ(expected_contents, test_file_contents);
}

// Mocks FileSelector used by FileHandlerSelectFileFunction.
// When |SelectFile| is called, it will check that file name suggestion is as
// expected, and respond to the extension function with specified selection
// results.
class MockFileSelector : public file_handler::FileSelector {
 public:
  MockFileSelector(const FilePath& suggested_name,
                   const std::vector<std::string>& allowed_extensions,
                   bool success,
                   const FilePath& selected_path)
      : suggested_name_(suggested_name),
        allowed_extensions_(allowed_extensions),
        success_(success),
        selected_path_(selected_path) {
  }
  virtual ~MockFileSelector() {}

  // file_handler::FileSelector implementation.
  // |browser| is not used.
  virtual void SelectFile(const FilePath& suggested_name,
                          const std::vector<std::string>& allowed_extensions,
                          Browser* browser,
                          FileHandlerSelectFileFunction* function) OVERRIDE {
    // Confirm that the function suggested us the right name.
    EXPECT_EQ(suggested_name_, suggested_name);
    // Confirm that the function allowed us the right extensions.
    EXPECT_EQ(allowed_extensions_.size(), allowed_extensions.size());
    if (allowed_extensions_.size() == allowed_extensions.size()) {
      for (size_t i = 0; i < allowed_extensions_.size(); ++i) {
        EXPECT_EQ(allowed_extensions_[i], allowed_extensions[i]);
      }
    }

    // Send response to the extension function.
    // The callback will take a reference to the function and keep it alive.
    base::MessageLoopProxy::current()->PostTask(FROM_HERE,
        base::Bind(&FileHandlerSelectFileFunction::OnFilePathSelected,
            function, success_, selected_path_));
    delete this;
  }

 private:
  // File name that is expected to be suggested by the function.
  FilePath suggested_name_;

  // Extensions that is expected to be allowed by the function.
  std::vector<std::string> allowed_extensions_;

  // Whether the selection should succeed.
  bool success_;
  // File path that should be returned to the function.
  FilePath selected_path_;

  DISALLOW_COPY_AND_ASSIGN(MockFileSelector);
};

// Mocks file selector factory for the test.
// When |CreateFileSelector| is invoked it will create mock file selector for
// the extension function with test parameters from the object ctor.
class MockFileSelectorFactory : public file_handler::FileSelectorFactory {
 public:
  explicit MockFileSelectorFactory(const TestCase& test_case)
      : suggested_name_(test_case.suggested_name),
        allowed_extensions_(test_case.allowed_extensions),
        success_(test_case.success),
        selected_path_(test_case.selected_path) {
  }
  virtual ~MockFileSelectorFactory() {}

  // file_handler::FileSelectorFactory imaplementation.
  virtual file_handler::FileSelector* CreateFileSelector() const OVERRIDE {
    return new MockFileSelector(suggested_name_,
                                allowed_extensions_,
                                success_,
                                selected_path_);
  }

 private:
  // File name that is expected to be suggested by the function.
  FilePath suggested_name_;
  // Extensions that is expected to be allowed by the function.
  std::vector<std::string> allowed_extensions_;
  // Whether the selection should succeed.
  bool success_;
  // File path that should be returned to the function.
  FilePath selected_path_;

  DISALLOW_COPY_AND_ASSIGN(MockFileSelectorFactory);
};

// Extension api test for the fileBrowserHandler extension API.
class FileBrowserHandlerExtensionTest : public ExtensionApiTest {
 protected:
  virtual void SetUp() OVERRIDE {
    // Create mount point directory that will be used in the test.
    // Mount point will be called "tmp", and it will be located in a tmp
    // directory with an unique name.
    FilePath tmp_dir_path;
    PathService::Get(base::DIR_TEMP, &tmp_dir_path);
    ASSERT_TRUE(scoped_tmp_dir_.CreateUniqueTempDirUnderPath(tmp_dir_path));
    tmp_mount_point_ = scoped_tmp_dir_.path().Append("tmp");
    file_util::CreateDirectory(tmp_mount_point_);

    ExtensionApiTest::SetUp();
  }

  // Creates new, test mount point.
  void AddTmpMountPoint() {
    fileapi::ExternalFileSystemMountPointProvider* provider =
        BrowserContext::GetFileSystemContext(browser()->profile())->
            external_provider();
    provider->AddLocalMountPoint(tmp_mount_point_);
  }

  FilePath GetFullPathOnTmpMountPoint(const FilePath& relative_path) {
    return tmp_mount_point_.Append(relative_path);
  }

  // Creates a new FileHandlerSelectFileFunction to be used in the test.
  // This function will be called from ExtensionFunctinoDispatcher whenever
  // an extension function for fileBrowserHandlerInternal.selectFile will be
  // needed.
  static ExtensionFunction* TestSelectFileFunctionFactory() {
    EXPECT_TRUE(test_cases_);
    EXPECT_TRUE(current_test_case_ < test_cases_->size());

    // If this happens, test failed. But, we still don't want to crash, so
    // return valid extension function.
    if (!test_cases_ && current_test_case_ >= test_cases_->size())
      return new FileHandlerSelectFileFunction();

    // Create file creator factory for the current test case.
    MockFileSelectorFactory* mock_factory =
        new MockFileSelectorFactory(test_cases_->at(current_test_case_));
    current_test_case_++;

    return new FileHandlerSelectFileFunction(mock_factory, false);
  }

  // Sets up test parameters for extension function invocations that will be
  // made during the test.
  void SetTestCases(const std::vector<TestCase>* test_cases) {
    test_cases_ = test_cases;
    current_test_case_ = 0;
  }

 private:
  // List of test parameters for each extension function invocation that will be
  // made during a test.
  // Should be owned by the test code.
  static const std::vector<TestCase>* test_cases_;
  static size_t current_test_case_;

  ScopedTempDir scoped_tmp_dir_;
  // Our test mount point path.
  FilePath tmp_mount_point_;
};

const std::vector<TestCase>* FileBrowserHandlerExtensionTest::test_cases_ =
    NULL;
size_t FileBrowserHandlerExtensionTest::current_test_case_ = 0;

// End to end test that verifies that fileBrowserHandler.selectFile works as
// expected. It will run test extension under
// chrome/test/data/extensions/api_test/file_browser/filehandler_create.
// The extension will invoke fileBrowserHandler.selectFile function twice.
// Once with suggested name "some_file_name.txt", and once with suggested name
// "fail". The file selection should succeed the first time, but fail the second
// time. When the file is selected the test extension will verify that it can
// create, read and write the file under the selected file path.
IN_PROC_BROWSER_TEST_F(FileBrowserHandlerExtensionTest, EndToEnd) {
  AddTmpMountPoint();

  // Path that will be "selected" by file selector.
  const FilePath selected_path =
      GetFullPathOnTmpMountPoint(FilePath("test_file.txt"));

  std::vector<std::string> allowed_extensions;
  allowed_extensions.push_back("txt");
  allowed_extensions.push_back("html");

  std::vector<TestCase> test_cases;
  test_cases.push_back(
      TestCase(FilePath("some_file_name.txt"),
               allowed_extensions,
               true,
               selected_path));
  test_cases.push_back(
      TestCase(FilePath("fail"),
               std::vector<std::string>(),
               false,
               FilePath()));

  SetTestCases(&test_cases);

  // Override extension function that will be used during the test.
  ASSERT_TRUE(ExtensionFunctionDispatcher::OverrideFunction(
      "fileBrowserHandlerInternal.selectFile",
      FileBrowserHandlerExtensionTest::TestSelectFileFunctionFactory));

  // Selected path should still not exist.
  ASSERT_FALSE(file_util::PathExists(selected_path));

  // Run the extension test.
  ASSERT_TRUE(RunExtensionTest("file_browser/filehandler_create")) << message_;

  // Selected path should have been created by the test extension after the
  // extension function call.
  ASSERT_TRUE(file_util::PathExists(selected_path));

  // Let's check that the file has the expected content.
  const std::string expected_contents = "hello from test extension.";
  content::BrowserThread::PostTask(content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&ExpectFileContentEquals, selected_path, expected_contents));

  // Make sure test doesn't finish until we check on file thread that the
  // selected file's content is as expected.
  content::RunAllPendingInMessageLoop(content::BrowserThread::FILE);

  SetTestCases(NULL);
}

// Tests that verifies the fileBrowserHandlerInternal.selectFile function fails
// when invoked without user gesture.
IN_PROC_BROWSER_TEST_F(FileBrowserHandlerExtensionTest, NoUserGesture) {
  scoped_refptr<FileHandlerSelectFileFunction> select_file_function(
      new FileHandlerSelectFileFunction());

  std::string error =
      utils::RunFunctionAndReturnError(
          select_file_function.get(),
          "[{\"suggestedName\": \"foo\"}]",
          browser());

  const std::string expected_error =
      "This method can only be called in response to user gesture, such as a "
      "mouse click or key press.";
  EXPECT_EQ(expected_error, error);
}

// Tests that checks that the fileHandlerInternal.selectFile function returns
// dictionary with |success == false| and no file entry when user cancels file
// selection.
IN_PROC_BROWSER_TEST_F(FileBrowserHandlerExtensionTest, SelectionFailed) {
  TestCase test_case(FilePath("some_file_name.txt"),
                     std::vector<std::string>(),
                     false,
                     FilePath());

  scoped_refptr<FileHandlerSelectFileFunction> select_file_function(
      new FileHandlerSelectFileFunction(new MockFileSelectorFactory(test_case),
                                        false));

  select_file_function->set_has_callback(true);
  select_file_function->set_user_gesture(true);

  scoped_ptr<base::DictionaryValue> result(utils::ToDictionary(
      utils::RunFunctionAndReturnSingleResult(
          select_file_function.get(),
          "[{\"suggestedName\": \"some_file_name.txt\"}]",
          browser())));

  EXPECT_FALSE(utils::GetBoolean(result.get(), "success"));
  DictionaryValue* entry_info;
  EXPECT_FALSE(result->GetDictionary("entry", &entry_info));
}

// Tests that user cannot be suggested a full file path when selecting a file,
// only a file name (i.e. that extension function caller has no influence on
// which directory contents will be initially displayed in selection dialog).
IN_PROC_BROWSER_TEST_F(FileBrowserHandlerExtensionTest, SuggestedFullPath) {
  TestCase test_case(FilePath("some_file_name.txt"),
                     std::vector<std::string>(),
                     false,
                     FilePath());

  scoped_refptr<FileHandlerSelectFileFunction> select_file_function(
      new FileHandlerSelectFileFunction(new MockFileSelectorFactory(test_case),
                                        false));

  select_file_function->set_has_callback(true);
  select_file_function->set_user_gesture(true);

  scoped_ptr<base::DictionaryValue> result(utils::ToDictionary(
      utils::RunFunctionAndReturnSingleResult(
          select_file_function.get(),
          "[{\"suggestedName\": \"/path_to_file/some_file_name.txt\"}]",
          browser())));

  EXPECT_FALSE(utils::GetBoolean(result.get(), "success"));
  DictionaryValue* entry_info;
  EXPECT_FALSE(result->GetDictionary("entry", &entry_info));
}

}  // namespace

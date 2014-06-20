// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/contact_info.h"

#include "base/basictypes.h"
#include "base/format_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/field_types.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;

namespace autofill {

struct FullNameTestCase {
  std::string full_name_input;
  std::string given_name_output;
  std::string middle_name_output;
  std::string family_name_output;
} full_name_test_cases[] = {
  { "", "", "", "" },
  { "John Smith", "John", "", "Smith" },
  { "Julien van der Poel", "Julien", "", "van der Poel" },
  { "John J Johnson", "John", "J", "Johnson" },
  { "John Smith, Jr.", "John", "", "Smith" },
  { "Mr John Smith", "John", "", "Smith" },
  { "Mr. John Smith", "John", "", "Smith" },
  { "Mr. John Smith, M.D.", "John", "", "Smith" },
  { "Mr. John Smith, MD", "John", "", "Smith" },
  { "Mr. John Smith MD", "John", "", "Smith" },
  { "William Hubert J.R.", "William", "Hubert", "J.R." },
  { "John Ma", "John", "", "Ma" },
  { "John Smith, MA", "John", "", "Smith" },
  { "John Jacob Jingleheimer Smith", "John Jacob", "Jingleheimer", "Smith" },
  { "Virgil", "Virgil", "", "" },
  { "Murray Gell-Mann", "Murray", "", "Gell-Mann" },
  { "Mikhail Yevgrafovich Saltykov-Shchedrin", "Mikhail", "Yevgrafovich",
    "Saltykov-Shchedrin" },
  { "Arthur Ignatius Conan Doyle", "Arthur Ignatius", "Conan", "Doyle" },
};

TEST(NameInfoTest, SetFullName) {
  for (size_t i = 0; i < arraysize(full_name_test_cases); ++i) {
    const FullNameTestCase& test_case = full_name_test_cases[i];
    SCOPED_TRACE(test_case.full_name_input);

    NameInfo name;
    name.SetInfo(AutofillType(NAME_FULL),
                 ASCIIToUTF16(test_case.full_name_input),
                 "en-US");
    EXPECT_EQ(ASCIIToUTF16(test_case.given_name_output),
              name.GetInfo(AutofillType(NAME_FIRST), "en-US"));
    EXPECT_EQ(ASCIIToUTF16(test_case.middle_name_output),
              name.GetInfo(AutofillType(NAME_MIDDLE), "en-US"));
    EXPECT_EQ(ASCIIToUTF16(test_case.family_name_output),
              name.GetInfo(AutofillType(NAME_LAST), "en-US"));
    EXPECT_EQ(ASCIIToUTF16(test_case.full_name_input),
              name.GetInfo(AutofillType(NAME_FULL), "en-US"));
  }
}

TEST(NameInfoTest, GetFullName) {
  NameInfo name;
  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, base::string16());
  name.SetRawInfo(NAME_LAST, base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First"));

  name.SetRawInfo(NAME_FIRST, base::string16());
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("Middle"));

  name.SetRawInfo(NAME_FIRST, base::string16());
  name.SetRawInfo(NAME_MIDDLE, base::string16());
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("Last"));

  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Middle"));

  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, base::string16());
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Last"));

  name.SetRawInfo(NAME_FIRST, base::string16());
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), base::string16());
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("Middle Last"));

  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  name.SetRawInfo(NAME_MIDDLE, ASCIIToUTF16("Middle"));
  name.SetRawInfo(NAME_LAST, ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Middle Last"));

  name.SetRawInfo(NAME_FULL, ASCIIToUTF16("First Middle Last, MD"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Middle Last, MD"));

  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("First"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("First Middle Last, MD"));

  // Changing something (e.g., the first name) clears the stored full name.
  name.SetRawInfo(NAME_FIRST, ASCIIToUTF16("Second"));
  EXPECT_EQ(name.GetRawInfo(NAME_FIRST), ASCIIToUTF16("Second"));
  EXPECT_EQ(name.GetRawInfo(NAME_MIDDLE), ASCIIToUTF16("Middle"));
  EXPECT_EQ(name.GetRawInfo(NAME_LAST), ASCIIToUTF16("Last"));
  EXPECT_EQ(name.GetRawInfo(NAME_FULL), ASCIIToUTF16("Second Middle Last"));
}

TEST(NameInfoTest, EqualsIgnoreCase) {
  struct TestCase {
    std::string starting_names[3];
    std::string additional_names[3];
    bool expected_result;
  };

  struct TestCase test_cases[] = {
      // Identical name comparison.
      {{"Marion", "Mitchell", "Morrison"},
       {"Marion", "Mitchell", "Morrison"},
       true},

      // Case-insensative comparisons.
      {{"Marion", "Mitchell", "Morrison"},
       {"Marion", "Mitchell", "MORRISON"},
       true},
      {{"Marion", "Mitchell", "Morrison"},
       {"MARION", "Mitchell", "MORRISON"},
       true},
      {{"Marion", "Mitchell", "Morrison"},
       {"MARION", "MITCHELL", "MORRISON"},
       true},
      {{"Marion", "", "Mitchell Morrison"},
       {"MARION", "", "MITCHELL MORRISON"},
       true},
      {{"Marion Mitchell", "", "Morrison"},
       {"MARION MITCHELL", "", "MORRISON"},
       true},

      // Identical full names but different canonical forms.
      {{"Marion", "Mitchell", "Morrison"},
       {"Marion", "", "Mitchell Morrison"},
       false},
      {{"Marion", "Mitchell", "Morrison"},
       {"Marion Mitchell", "", "MORRISON"},
       false},

      // Different names.
      {{"Marion", "Mitchell", "Morrison"}, {"Marion", "M.", "Morrison"}, false},
      {{"Marion", "Mitchell", "Morrison"}, {"MARION", "M.", "MORRISON"}, false},
      {{"Marion", "Mitchell", "Morrison"},
       {"David", "Mitchell", "Morrison"},
       false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(test_cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("i: %" PRIuS, i));

    // Construct the starting_profile.
    NameInfo starting_profile;
    starting_profile.SetRawInfo(NAME_FIRST,
                                ASCIIToUTF16(test_cases[i].starting_names[0]));
    starting_profile.SetRawInfo(NAME_MIDDLE,
                                ASCIIToUTF16(test_cases[i].starting_names[1]));
    starting_profile.SetRawInfo(NAME_LAST,
                                ASCIIToUTF16(test_cases[i].starting_names[2]));

    // Construct the additional_profile.
    NameInfo additional_profile;
    additional_profile.SetRawInfo(
        NAME_FIRST, ASCIIToUTF16(test_cases[i].additional_names[0]));
    additional_profile.SetRawInfo(
        NAME_MIDDLE, ASCIIToUTF16(test_cases[i].additional_names[1]));
    additional_profile.SetRawInfo(
        NAME_LAST, ASCIIToUTF16(test_cases[i].additional_names[2]));

    // Verify the test expectations.
    EXPECT_EQ(test_cases[i].expected_result,
              starting_profile.EqualsIgnoreCase(additional_profile));
  }
}

}  // namespace autofill

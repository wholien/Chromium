// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/test_runner/mock_grammar_check.h"

#include <stddef.h>

#include <algorithm>

#include "base/logging.h"
#include "base/macros.h"
#include "components/test_runner/test_common.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebTextCheckingResult.h"

namespace test_runner {

bool MockGrammarCheck::CheckGrammarOfString(
    const blink::WebString& text,
    std::vector<blink::WebTextCheckingResult>* results) {
  DCHECK(results);
  base::string16 string_text = text.utf16();
  if (std::find_if(string_text.begin(), string_text.end(), IsASCIIAlpha) ==
      string_text.end())
    return true;

  // Find matching grammatical errors from known ones. This function has to
  // check all errors because the given text may consist of two or more
  // sentences that have grammatical errors.
  static const struct {
    const char* text;
    int location;
    int length;
  } kGrammarErrors[] = {
        {"I have a issue.", 7, 1},
        {"I have an grape.", 7, 2},
        {"I have an kiwi.", 7, 2},
        {"I have an muscat.", 7, 2},
        {"You has the right.", 4, 3},
        {"apple orange zz.", 0, 16},
        {"apple zz orange.", 0, 16},
        {"apple,zz,orange.", 0, 16},
        {"orange,zz,apple.", 0, 16},
        {"the the adlj adaasj sdklj. there there", 4, 3},
        {"the the adlj adaasj sdklj. there there", 33, 5},
        {"zz apple orange.", 0, 16},
    };
  for (size_t i = 0; i < arraysize(kGrammarErrors); ++i) {
    size_t offset = 0;
    base::string16 error(
        kGrammarErrors[i].text,
        kGrammarErrors[i].text + strlen(kGrammarErrors[i].text));
    while ((offset = string_text.find(error, offset)) != base::string16::npos) {
      results->push_back(
          blink::WebTextCheckingResult(blink::WebTextDecorationTypeGrammar,
                                       offset + kGrammarErrors[i].location,
                                       kGrammarErrors[i].length));
      offset += kGrammarErrors[i].length;
    }
  }
  return false;
}

}  // namespace test_runner

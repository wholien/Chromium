// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_TEST_HELPER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_TEST_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/media/router/issues_observer.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media_router {

// Matcher for objects that uses Equals() member function for equality check.
MATCHER_P(Equals, other, "") {
  return arg.Equals(other);
}

// Matcher for a sequence of objects that uses Equals() member function for
// equality check.
MATCHER_P(SequenceEquals, other, "") {
  if (arg.size() != other.size()) {
    return false;
  }
  for (size_t i = 0; i < arg.size(); ++i) {
    if (!arg[i].Equals(other[i])) {
      return false;
    }
  }
  return true;
}

// Matcher for IssueInfo title.
MATCHER_P(IssueTitleEquals, title, "") {
  return arg.title == title;
}

MATCHER_P(StateChangeInfoEquals, other, "") {
  return arg.state == other.state && arg.close_reason == other.close_reason &&
         arg.message == other.message;
}

class MockIssuesObserver : public IssuesObserver {
 public:
  explicit MockIssuesObserver(MediaRouter* router);
  ~MockIssuesObserver() override;

  MOCK_METHOD1(OnIssue, void(const Issue& issue));
  MOCK_METHOD0(OnIssuesCleared, void());
};

class MockMediaSinksObserver : public MediaSinksObserver {
 public:
  MockMediaSinksObserver(MediaRouter* router,
                         const MediaSource& source,
                         const url::Origin& origin);
  ~MockMediaSinksObserver() override;

  MOCK_METHOD1(OnSinksReceived, void(const std::vector<MediaSink>& sinks));
};

class MockMediaRoutesObserver : public MediaRoutesObserver {
 public:
  explicit MockMediaRoutesObserver(MediaRouter* router,
      const MediaSource::Id source_id = std::string());
  ~MockMediaRoutesObserver() override;

  MOCK_METHOD2(OnRoutesUpdated, void(const std::vector<MediaRoute>& routes,
      const std::vector<MediaRoute::Id>& joinable_route_ids));
};

class MockPresentationConnectionStateChangedCallback {
 public:
  MockPresentationConnectionStateChangedCallback();
  ~MockPresentationConnectionStateChangedCallback();
  MOCK_METHOD1(Run,
               void(const content::PresentationConnectionStateChangeInfo&));
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_TEST_HELPER_H_

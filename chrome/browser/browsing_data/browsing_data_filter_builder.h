// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_FILTER_BUILDER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_FILTER_BUILDER_H_

#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"

class GURL;

namespace net {
class CanonicalCookie;
}

namespace url {
class Origin;
}

// An class that builds GURL->bool predicates to filter browsing data.
// These filters can be of two modes - a whitelist or a blacklist. The filter
// entries can be origins or registrable domains.
//
// This class defines interface to build filters for various kinds of browsing
// data. |BuildGeneralFilter()| is useful for most browsing data storage
// backends, but some backends, such as website settings and cookies, use
// other formats of filter.
class BrowsingDataFilterBuilder {
 public:
  enum Mode {
    // This means that only the origins given will be deleted.
    WHITELIST,
    // Everyone EXCEPT the origins given will be deleted.
    BLACKLIST
  };

  // Constructs a filter with the given |mode| - whitelist or blacklist.
  static std::unique_ptr<BrowsingDataFilterBuilder> Create(Mode mode);

  virtual ~BrowsingDataFilterBuilder() = default;

  // Adds an origin to the filter. Note that this makes it impossible to
  // create cookie, channel ID, or plugin filters, as those datatypes are
  // scoped more broadly than an origin.
  virtual void AddOrigin(const url::Origin& origin) = 0;

  // Adds a registrable domain (e.g. google.com), an internal hostname
  // (e.g. localhost), or an IP address (e.g. 127.0.0.1). Other domains, such
  // as third and lower level domains (e.g. www.google.com) are not accepted.
  // Formally, it must hold that GetDomainAndRegistry(|registrable_domain|, _)
  // is |registrable_domain| itself or an empty string for this method
  // to accept it.
  virtual void AddRegisterableDomain(const std::string& registrable_domain) = 0;

  // Returns true if we're an empty blacklist, where we delete everything.
  virtual bool IsEmptyBlacklist() const = 0;

  // Builds a filter that matches URLs that are in the whitelist,
  // or aren't in the blacklist.
  virtual base::RepeatingCallback<bool(const GURL&)>
      BuildGeneralFilter() const = 0;

  // Builds a filter that matches cookies whose sources are in the whitelist,
  // or aren't in the blacklist.
  virtual base::RepeatingCallback<bool(const net::CanonicalCookie& pattern)>
      BuildCookieFilter() const = 0;

  // Builds a filter that matches channel IDs whose server identifiers are in
  // the whitelist, or aren't in the blacklist.
  virtual base::RepeatingCallback<bool(const std::string& server_id)>
      BuildChannelIDFilter() const = 0;

  // Builds a filter that matches the |site| of a plugin.
  virtual base::RepeatingCallback<bool(const std::string& site)>
      BuildPluginFilter() const = 0;

  // A convenience method to produce an empty blacklist, a filter that matches
  // everything.
  static base::Callback<bool(const GURL&)> BuildNoopFilter();

  // The mode of the filter.
  virtual Mode GetMode() const = 0;

  // Create a new filter builder with the same set of origins, set of domains,
  // and mode.
  virtual std::unique_ptr<BrowsingDataFilterBuilder> Copy() const = 0;

  // Comparison.
  virtual bool operator==(const BrowsingDataFilterBuilder& other) const = 0;
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_FILTER_BUILDER_H_

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('md_history.history_routing_test', function() {
  function registerTests() {
    suite('routing-test', function() {
      var app;
      var list;
      var toolbar;

      function navigateTo(route) {
        window.history.replaceState({}, '', route);
        window.dispatchEvent(new CustomEvent('location-changed'));
      }

      setup(function() {
        assertEquals('chrome://history/', window.location.href);
        app = replaceApp();
        sidebar = app.$['content-side-bar']
        toolbar = app.$['toolbar'];
      });

      test('changing route changes active view', function() {
        assertEquals('history', app.$.content.selected);
        navigateTo('/syncedTabs');
        return PolymerTest.flushTasks().then(function() {
          assertEquals('syncedTabs', app.$.content.selected);
          assertEquals('chrome://history/syncedTabs', window.location.href);
        });
      });

      test('route updates from sidebar', function() {
        var menu = sidebar.$.menu;
        assertEquals('history', app.selectedPage_);
        assertEquals('chrome://history/', window.location.href);

        MockInteractions.tap(menu.children[1]);
        assertEquals('syncedTabs', app.selectedPage_);
        assertEquals('chrome://history/syncedTabs', window.location.href);

        MockInteractions.keyDownOn(menu.children[0], 32, '', "Space");
        assertEquals('history', app.selectedPage_);
        assertEquals('chrome://history/', window.location.href);
      });

      test('search updates from route', function() {
        assertEquals('chrome://history/', window.location.href);
        var searchTerm = 'Mei';
        assertEquals('history', app.$.content.selected);
        navigateTo('/?q=' + searchTerm);
        assertEquals(searchTerm, toolbar.searchTerm);
      });

      test('route updates from search', function() {
        var searchTerm = 'McCree';
        assertEquals('history', app.$.content.selected);
        app.fire('change-query', {search: searchTerm});
        assertEquals('chrome://history/?q=' + searchTerm, window.location.href);
      });

      test('search preserved across menu items', function() {
        var searchTerm = 'Soldier76';
        var menu = sidebar.$.menu;
        assertEquals('history', app.selectedPage_);
        navigateTo('/?q=' + searchTerm);

        MockInteractions.tap(menu.children[1]);
        assertEquals('syncedTabs', app.selectedPage_);
        assertEquals(searchTerm, toolbar.searchTerm);
        assertEquals(
            'chrome://history/syncedTabs?q=' + searchTerm,
            window.location.href);

        MockInteractions.tap(menu.children[0]);
        assertEquals('history', app.selectedPage_);
        assertEquals(searchTerm, toolbar.searchTerm);
        assertEquals('chrome://history/?q=' + searchTerm, window.location.href);
      });

      teardown(function() {
        // Reset back to initial navigation state.
        navigateTo('/');
      });
    });
  }
  return {
    registerTests: registerTests
  };
});

cr.define('md_history.history_routing_test_with_query_param', function() {
  function registerTests() {
    suite('routing-with-query-param', function() {
      var app;
      var toolbar;
      var expectedQuery;

      suiteSetup(function() {
        app = $('history-app');
        toolbar = app.$['toolbar'];
        expectedQuery = 'query';
      });

      test('search initiated on load', function(done) {
        var verifyFunction = function(info) {
          assertEquals(expectedQuery, info[0]);
          PolymerTest.flushTasks().then(function() {
            assertEquals(
                expectedQuery,
                toolbar.$['main-toolbar'].getSearchField().getValue());
            done();
          });
        };

        if (window.historyQueryInfo) {
          verifyFunction(window.historyQueryInfo);
          return;
        }

        registerMessageCallback('queryHistory', this, verifyFunction);
      });
    });
  }
  return {
    registerTests: registerTests
  };
});

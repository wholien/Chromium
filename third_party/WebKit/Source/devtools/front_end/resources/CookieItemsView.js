/*
 * Copyright (C) 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @unrestricted
 */
Resources.CookieItemsView = class extends UI.SimpleView {
  constructor(treeElement, target, cookieDomain) {
    super(Common.UIString('Cookies'));

    this.element.classList.add('storage-view');

    this._deleteButton = new UI.ToolbarButton(Common.UIString('Delete Selected'), 'largeicon-delete');
    this._deleteButton.addEventListener(UI.ToolbarButton.Events.Click, this._deleteButtonClicked, this);

    this._clearButton = new UI.ToolbarButton(Common.UIString('Clear All'), 'largeicon-clear');
    this._clearButton.addEventListener(UI.ToolbarButton.Events.Click, this._clearButtonClicked, this);

    this._refreshButton = new UI.ToolbarButton(Common.UIString('Refresh'), 'largeicon-refresh');
    this._refreshButton.addEventListener(UI.ToolbarButton.Events.Click, this._refreshButtonClicked, this);

    this._filterBar = new UI.FilterBar('cookiesPanel', true);
    this._textFilterUI = new UI.TextFilterUI(true);
    this._textFilterUI.addEventListener(UI.FilterUI.Events.FilterChanged, this._filterChanged, this);
    this._filterBar.addFilter(this._textFilterUI);

    this._filterSeparator = new UI.ToolbarSeparator();
    this._filterButton = this._filterBar.filterButton();

    this._target = target;
    this._treeElement = treeElement;
    this._cookieDomain = cookieDomain;

    this.element.addEventListener('contextmenu', this._contextMenu.bind(this), true);
  }

  /**
   * @override
   * @return {!Array.<!UI.ToolbarItem>}
   */
  syncToolbarItems() {
    return [this._refreshButton, this._clearButton, this._deleteButton, this._filterSeparator, this._filterButton];
  }

  /**
   * @override
   */
  wasShown() {
    this._update();
  }

  /**
   * @override
   */
  willHide() {
    this._deleteButton.setEnabled(false);
  }

  /**
   * @param {!Common.Event} event
   */
  _filterChanged(event) {
    var text = this._textFilterUI.value();
    this._filterRegex = text && new RegExp(text.escapeForRegExp(), 'i');
    this._update();
  }

  _update() {
    var resourceURLs = [];
    var cookieDomain = this._cookieDomain;
    /**
     * @param {!SDK.Resource} resource
     */
    function populateResourceURLs(resource) {
      var url = resource.documentURL.asParsedURL();
      if (url && url.securityOrigin() === cookieDomain)
        resourceURLs.push(resource.url);
    }

    SDK.ResourceTreeModel.fromTarget(this._target).forAllResources(populateResourceURLs);
    SDK.Cookies.getCookiesAsync(this._target, resourceURLs, this._updateWithCookies.bind(this));
  }

  /**
   * @param {!Array.<!SDK.Cookie>} allCookies
   */
  _updateWithCookies(allCookies) {
    this._cookies = allCookies;
    this._totalSize = allCookies.reduce((size, cookie) => size + cookie.size(), 0);

    if (!this._cookiesTable) {
      const parsedURL = this._cookieDomain.asParsedURL();
      const domain = parsedURL ? parsedURL.host : '';
      this._cookiesTable =
          new CookieTable.CookiesTable(false, this._update.bind(this), this._enableDeleteButton.bind(this), domain);
    }

    var shownCookies = this._filterCookies(this._cookies);
    this._cookiesTable.setCookies(shownCookies);
    this._filterBar.show(this.element);
    this._cookiesTable.show(this.element);
    this._treeElement.subtitle =
        String.sprintf(Common.UIString('%d cookies (%s)'), this._cookies.length, Number.bytesToString(this._totalSize));
    this._filterButton.setEnabled(true);
    this._clearButton.setEnabled(true);
    this._deleteButton.setEnabled(!!this._cookiesTable.selectedCookie());
  }

  /**
   * @param {!Array.<!SDK.Cookie>} cookies
   */
  _filterCookies(cookies) {
    if (!this._filterRegex)
      return cookies;

    return cookies.filter(cookie => {
      const candidate = `${cookie.name()} ${cookie.value()} ${cookie.domain()}`;
      return this._filterRegex.test(candidate);
    });
  }

  clear() {
    this._cookiesTable.clear();
    this._update();
  }

  /**
   * @param {!Common.Event} event
   */
  _clearButtonClicked(event) {
    this.clear();
  }

  _enableDeleteButton() {
    this._deleteButton.setEnabled(true);
  }

  /**
   * @param {!Common.Event} event
   */
  _deleteButtonClicked(event) {
    var selectedCookie = this._cookiesTable.selectedCookie();
    if (selectedCookie) {
      selectedCookie.remove();
      this._update();
    }
  }

  /**
   * @param {!Common.Event} event
   */
  _refreshButtonClicked(event) {
    this._update();
  }

  _contextMenu(event) {
    if (!this._cookies.length) {
      var contextMenu = new UI.ContextMenu(event);
      contextMenu.appendItem(Common.UIString('Refresh'), this._update.bind(this));
      contextMenu.show();
    }
  }
};

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Behavior for scrollable containers with <iron-list>.
 *
 * Any containers with the 'scrollable' attribute set will have the following
 * classes toggled appropriately: can-scroll, is-scrolled, scrolled-to-bottom.
 * These classes are used to style the container div and list elements
 * appropriately, see shared_style_css.html.
 *
 * The associated HTML should look something like:
 *   <div id="container" scrollable>
 *     <iron-list items="[[items]]" scroll-target="container">
 *       <template>
 *         <my-element item="[[item]] tabindex$="[[tabIndex]]"></my-element>
 *       </template>
 *     </iron-list>
 *   </div>
 *
 * In order to get correct keyboard focus (tab) behavior within the list,
 * any elements with tabbable sub-elements also need to set tabindex, e.g:
 *
 * <dom-module id="my-element>
 *   <template>
 *     ...
 *     <paper-icon-button toggles active="{{opened}}" tabindex$="[[tabindex]]">
 *   </template>
 * </dom-module>
 *
 * NOTE: If 'container' is not fixed size, it is important to call
 * updateScrollableContents() when [[items]] changes, otherwise the container
 * will not be sized correctly.
 */

/** @polymerBehavior */
var CrScrollableBehavior = {
  properties: {
    /** @private {number|null} */
    intervalId_: {type: Number, value: null}
  },

  ready: function() {
    var scrollableElements = this.root.querySelectorAll('[scrollable]');

    // Setup the intial scrolling related classes for each scrollable container.
    requestAnimationFrame(function() {
      for (var i = 0; i < scrollableElements.length; i++)
        this.updateScroll_(scrollableElements[i]);
    }.bind(this));

    // Listen to the 'scroll' event for each scrollable container.
    for (var i = 0; i < scrollableElements.length; i++) {
      scrollableElements[i].addEventListener(
          'scroll', this.updateScrollEvent_.bind(this));
    }
  },

  detached: function() {
    if (this.intervalId_ !== null)
      clearInterval(this.intervalId_);
  },

  /**
   * Called any time the contents of a scrollable container may have changed.
   * This ensures that the <iron-list> contents of dynamically sized
   * containers are resized correctly.
   */
  updateScrollableContents: function() {
    if (this.intervalId_ !== null)
      return;  // notifyResize is arelady in progress.

    var nodeList = this.root.querySelectorAll('[scrollable] iron-list');
    // Use setInterval to avoid initial render / sizing issues.
    this.intervalId_ = window.setInterval(function() {
      var unreadyNodes = [];
      for (var i = 0; i < nodeList.length; i++) {
        var node = nodeList[i];
        if (node.parentNode.scrollHeight == 0) {
          unreadyNodes.push(node);
          continue;
        }
        var ironList = /** @type {!IronListElement} */ (node);
        ironList.notifyResize();
      }
      if (unreadyNodes.length == 0) {
        window.clearInterval(this.intervalId_);
        this.intervalId_ = null;
      } else {
        nodeList = unreadyNodes;
      }
    }.bind(this), 10);
  },

  /**
   * Event wrapper for updateScroll_.
   * @param {!Event} event
   * @private
   */
  updateScrollEvent_: function(event) {
    var scrollable = /** @type {!HTMLElement} */ (event.target);
    this.updateScroll_(scrollable);
  },

  /**
   * This gets called once intially and any time a scrollable container scrolls.
   * @param {!HTMLElement} scrollable
   * @private
   */
  updateScroll_: function(scrollable) {
    scrollable.classList.toggle(
        'can-scroll', scrollable.clientHeight < scrollable.scrollHeight);
    scrollable.classList.toggle('is-scrolled', scrollable.scrollTop > 0);
    scrollable.classList.toggle(
        'scrolled-to-bottom', scrollable.scrollTop + scrollable.clientHeight >=
            scrollable.scrollHeight);
  },
};

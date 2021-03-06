/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

// No sense updating the overview more often than receiving data from the
// backend. Make sure this isn't lower than DEFAULT_TIMELINE_DATA_PULL_TIMEOUT
// in toolkit/devtools/server/actors/timeline.js
const OVERVIEW_UPDATE_INTERVAL = 200; // ms

const FRAMERATE_GRAPH_HEIGHT = 60; // px
const GRAPH_SCROLL_EVENTS_DRAIN = 50; // ms

/**
 * View handler for the overview panel's time view, displaying
 * framerate over time.
 */
let OverviewView = {
  /**
   * Sets up the view with event binding.
   */
  initialize: Task.async(function *() {
    this._framerateEl = $("#time-framerate");
    this._ticksData = [];

    this._start = this._start.bind(this);
    this._stop = this._stop.bind(this);
    this._onTimelineData = this._onTimelineData.bind(this);
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onGraphMouseUp = this._onGraphMouseUp.bind(this);
    this._onGraphScroll = this._onGraphScroll.bind(this);

    yield this._initializeFramerateGraph();

    this.framerateGraph.on("mouseup", this._onGraphMouseUp);
    this.framerateGraph.on("scroll", this._onGraphScroll);
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);
    PerformanceController.on(EVENTS.TIMELINE_DATA, this._onTimelineData);
  }),

  /**
   * Unbinds events.
   */
  destroy: function () {
    this.framerateGraph.off("mouseup", this._onGraphMouseUp);
    this.framerateGraph.off("scroll", this._onGraphScroll);
    clearNamedTimeout("graph-scroll");
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._start);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
    PerformanceController.off(EVENTS.TIMELINE_DATA, this._onTimelineData);
  },

  /**
   * Called at most every OVERVIEW_UPDATE_INTERVAL milliseconds
   * and uses data fetched from `_onTimelineData` to render
   * data into all the corresponding overview graphs.
   */
  _onRecordingTick: Task.async(function *() {
    // The `ticks` event on the TimelineFront returns all ticks for the
    // recording session, so just convert to plottable values and draw.
    let [, timestamps] = this._ticksData;
    yield this.framerateGraph.setDataFromTimestamps(timestamps);

    this.emit(EVENTS.OVERVIEW_RENDERED);
    this._prepareNextTick();
  }),

  /**
   * Fired when the graph selection has changed. Called by
   * mouseup and scroll events.
   */
  _onSelectionChange: function () {
    if (this.framerateGraph.hasSelection()) {
      let { min: beginAt, max: endAt } = this.framerateGraph.getMappedSelection();
      this.emit(EVENTS.OVERVIEW_RANGE_SELECTED, { beginAt, endAt });
    } else {
      this.emit(EVENTS.OVERVIEW_RANGE_CLEARED);
    }
  },

  /**
   * Listener handling the "mouseup" event for the framerate graph.
   * Fires an event to be handled elsewhere.
   */
  _onGraphMouseUp: function () {
    this._onSelectionChange();
  },

  /**
   * Listener handling the "scroll" event for the framerate graph.
   * Fires an event to be handled elsewhere.
   */
  _onGraphScroll: function () {
    setNamedTimeout("graph-scroll", GRAPH_SCROLL_EVENTS_DRAIN, () => {
      this._onSelectionChange();
    });
  },

  /**
   * Sets up the framerate graph.
   */
  _initializeFramerateGraph: Task.async(function *() {
    let graph = new LineGraphWidget(this._framerateEl, L10N.getStr("graphs.fps"));
    graph.fixedHeight = FRAMERATE_GRAPH_HEIGHT;
    graph.selectionEnabled = false;
    this.framerateGraph = graph;

    yield graph.ready();
  }),

  /**
   * Called to refresh the timer to keep firing _onRecordingTick.
   */
  _prepareNextTick: function () {
    // Check here to see if there's still a _timeoutId, incase
    // `stop` was called before the _prepareNextTick call was executed.
    if (this._timeoutId) {
      this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    }
  },

  /**
   * Event handlers
   */

  _start: function () {
    this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    this.framerateGraph.dropSelection();
  },

  _stop: function () {
    clearTimeout(this._timeoutId);
    this.framerateGraph.selectionEnabled = true;
  },

  /**
   * Called when the TimelineFront has new data for
   * framerate, markers or memory, and stores the data
   * to be plotted subsequently.
   */
  _onTimelineData: function (_, eventName, ...data) {
    if (eventName === "ticks") {
      this._ticksData = data;
    }
  }
};

// Decorates the OverviewView as an EventEmitter
EventEmitter.decorate(OverviewView);

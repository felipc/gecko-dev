/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layers_InputBlockState_h
#define mozilla_layers_InputBlockState_h

#include "nsTArray.h"                       // for nsTArray
#include "InputData.h"                      // for MultiTouchInput
#include "nsAutoPtr.h"

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class OverscrollHandoffChain;
class CancelableBlockState;
class TouchBlockState;
class WheelBlockState;

/**
 * A base class that stores state common to various input blocks.
 * Currently, it just stores the overscroll handoff chain.
 */
class InputBlockState
{
public:
  static const uint64_t NO_BLOCK_ID = 0;

  explicit InputBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                           bool aTargetConfirmed);
  virtual ~InputBlockState()
  {}

  bool SetConfirmedTargetApzc(const nsRefPtr<AsyncPanZoomController>& aTargetApzc);
  const nsRefPtr<AsyncPanZoomController>& GetTargetApzc() const;
  const nsRefPtr<const OverscrollHandoffChain>& GetOverscrollHandoffChain() const;
  uint64_t GetBlockId() const;

  bool IsTargetConfirmed() const;

private:
  nsRefPtr<AsyncPanZoomController> mTargetApzc;
  nsRefPtr<const OverscrollHandoffChain> mOverscrollHandoffChain;
  bool mTargetConfirmed;
  const uint64_t mBlockId;
};

/**
 * This class represents a set of events that can be cancelled by web content
 * via event listeners.
 *
 * Each cancelable input block can be cancelled by web content, and
 * this information is stored in the mPreventDefault flag. Because web
 * content runs on the Gecko main thread, we cannot always wait for web content's
 * response. Instead, there is a timeout that sets this flag in the case
 * where web content doesn't respond in time. The mContentResponded
 * and mContentResponseTimerExpired flags indicate which of these scenarios
 * occurred.
 */
class CancelableBlockState : public InputBlockState
{
public:
  CancelableBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                       bool aTargetConfirmed);

  virtual TouchBlockState *AsTouchBlock() {
    return nullptr;
  }
  virtual WheelBlockState *AsWheelBlock() {
    return nullptr;
  }

  /**
   * Record whether or not content cancelled this block of events.
   * @param aPreventDefault true iff the block is cancelled.
   * @return false if this block has already received a response from
   *         web content, true if not.
   */
  bool SetContentResponse(bool aPreventDefault);

  /**
   * Record that content didn't respond in time.
   * @return false if this block already timed out, true if not.
   */
  bool TimeoutContentResponse();

  /**
   * @return true iff web content cancelled this block of events.
   */
  bool IsDefaultPrevented() const;

  /**
   * @return true iff this block has received all the information needed
   *         to properly dispatch the events in the block.
   */
  virtual bool IsReadyForHandling() const;

  /**
   * Returns whether or not this block has pending events.
   */
  virtual bool HasEvents() const = 0;

  /**
   * Throw away all the events in this input block.
   */
  virtual void DropEvents() = 0;

  /**
   * Process all events given an apzc, leaving ths block depleted.
   */
  virtual void HandleEvents(const nsRefPtr<AsyncPanZoomController>& aTarget) = 0;

  /**
   * Return true if this input block must stay active if it would otherwise
   * be removed as the last item in the pending queue.
   */
  virtual bool MustStayActive() = 0;

  /**
   * Return a descriptive name for the block kind.
   */
  virtual const char* Type() = 0;

private:
  bool mPreventDefault;
  bool mContentResponded;
  bool mContentResponseTimerExpired;
};

/**
 * A single block of wheel events.
 */
class WheelBlockState : public CancelableBlockState
{
public:
  WheelBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                  bool aTargetConfirmed);

  bool IsReadyForHandling() const MOZ_OVERRIDE;
  bool HasEvents() const MOZ_OVERRIDE;
  void DropEvents() MOZ_OVERRIDE;
  void HandleEvents(const nsRefPtr<AsyncPanZoomController>& aTarget) MOZ_OVERRIDE;
  bool MustStayActive() MOZ_OVERRIDE;
  const char* Type() MOZ_OVERRIDE;

  void AddEvent(const ScrollWheelInput& aEvent);

  WheelBlockState *AsWheelBlock() MOZ_OVERRIDE {
    return this;
  }

private:
  nsTArray<ScrollWheelInput> mEvents;
};

/**
 * This class represents a single touch block. A touch block is
 * a set of touch events that can be cancelled by web content via
 * touch event listeners.
 *
 * Every touch-start event creates a new touch block. In this case, the
 * touch block consists of the touch-start, followed by all touch events
 * up to but not including the next touch-start (except in the case where
 * a long-tap happens, see below). Note that in particular we cannot know
 * when a touch block ends until the next one is started. Most touch
 * blocks are created by receipt of a touch-start event.
 *
 * Every long-tap event also creates a new touch block, since it can also
 * be consumed by web content. In this case, when the long-tap event is
 * dispatched to web content, a new touch block is started to hold the remaining
 * touch events, up to but not including the next touch start (or long-tap).
 *
 * Additionally, if touch-action is enabled, each touch block should
 * have a set of allowed touch behavior flags; one for each touch point.
 * This also requires running code on the Gecko main thread, and so may
 * be populated with some latency. The mAllowedTouchBehaviorSet and
 * mAllowedTouchBehaviors variables track this information.
 */
class TouchBlockState : public CancelableBlockState
{
public:
  typedef uint32_t TouchBehaviorFlags;

  explicit TouchBlockState(const nsRefPtr<AsyncPanZoomController>& aTargetApzc,
                           bool aTargetConfirmed);

  TouchBlockState *AsTouchBlock() MOZ_OVERRIDE {
    return this;
  }

  /**
   * Set the allowed touch behavior flags for this block.
   * @return false if this block already has these flags set, true if not.
   */
  bool SetAllowedTouchBehaviors(const nsTArray<TouchBehaviorFlags>& aBehaviors);
  /**
   * Copy the allowed touch behavior flags from another block.
   * @return false if this block already has these flags set, true if not.
   */
  bool CopyAllowedTouchBehaviorsFrom(const TouchBlockState& aOther);

  /**
   * @return true iff this block has received all the information needed
   *         to properly dispatch the events in the block.
   */
  bool IsReadyForHandling() const MOZ_OVERRIDE;

  /**
   * Sets a flag that indicates this input block occurred while the APZ was
   * in a state of fast motion. This affects gestures that may be produced
   * from input events in this block.
   */
  void SetDuringFastMotion();
  /**
   * @return true iff SetDuringFastMotion was called on this block.
   */
  bool IsDuringFastMotion() const;
  /**
   * Set the single-tap-occurred flag that indicates that this touch block
   * triggered a single tap event.
   * @return true if the flag was set. This may not happen if, for example,
   *         SetDuringFastMotion was previously called.
   */
  bool SetSingleTapOccurred();
  /**
   * @return true iff the single-tap-occurred flag is set on this block.
   */
  bool SingleTapOccurred() const;

  /**
   * Add a new touch event to the queue of events in this input block.
   */
  void AddEvent(const MultiTouchInput& aEvent);

  /**
   * @return false iff touch-action is enabled and the allowed touch behaviors for
   *         this touch block do not allow pinch-zooming.
   */
  bool TouchActionAllowsPinchZoom() const;
  /**
   * @return false iff touch-action is enabled and the allowed touch behaviors for
   *         this touch block do not allow double-tap zooming.
   */
  bool TouchActionAllowsDoubleTapZoom() const;
  /**
   * @return false iff touch-action is enabled and the allowed touch behaviors for
   *         the first touch point do not allow panning in the specified direction(s).
   */
  bool TouchActionAllowsPanningX() const;
  bool TouchActionAllowsPanningY() const;
  bool TouchActionAllowsPanningXY() const;

  bool HasEvents() const MOZ_OVERRIDE;
  void DropEvents() MOZ_OVERRIDE;
  void HandleEvents(const nsRefPtr<AsyncPanZoomController>& aTarget) MOZ_OVERRIDE;
  bool MustStayActive() MOZ_OVERRIDE;
  const char* Type() MOZ_OVERRIDE;

private:
  /**
   * @return the first event in the queue. The event is removed from the queue
   *         before it is returned.
   */
  MultiTouchInput RemoveFirstEvent();

private:
  nsTArray<TouchBehaviorFlags> mAllowedTouchBehaviors;
  bool mAllowedTouchBehaviorSet;
  bool mDuringFastMotion;
  bool mSingleTapOccurred;
  nsTArray<MultiTouchInput> mEvents;
};

} // namespace layers
} // namespace mozilla

#endif // mozilla_layers_InputBlockState_h

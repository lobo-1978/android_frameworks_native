/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIBINPUT_INPUT_H
#define _LIBINPUT_INPUT_H

#pragma GCC system_header

/**
 * Native input event structures.
 */

#include <android/input.h>
#ifdef __linux__
#include <android/os/IInputConstants.h>
#endif
#include <math.h>
#include <stdint.h>
#include <ui/Transform.h>
#include <utils/BitSet.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>
#include <array>
#include <limits>
#include <queue>

/*
 * Additional private constants not defined in ndk/ui/input.h.
 */
enum {
#ifdef __linux__
    /* This event was generated or modified by accessibility service. */
    AKEY_EVENT_FLAG_IS_ACCESSIBILITY_EVENT =
            android::os::IInputConstants::INPUT_EVENT_FLAG_IS_ACCESSIBILITY_EVENT, // 0x800,
#else
    AKEY_EVENT_FLAG_IS_ACCESSIBILITY_EVENT = 0x800,
#endif
    /* Signifies that the key is being predispatched */
    AKEY_EVENT_FLAG_PREDISPATCH = 0x20000000,

    /* Private control to determine when an app is tracking a key sequence. */
    AKEY_EVENT_FLAG_START_TRACKING = 0x40000000,

    /* Key event is inconsistent with previously sent key events. */
    AKEY_EVENT_FLAG_TAINTED = 0x80000000,
};

enum {

    /**
     * This flag indicates that the window that received this motion event is partly
     * or wholly obscured by another visible window above it.  This flag is set to true
     * even if the event did not directly pass through the obscured area.
     * A security sensitive application can check this flag to identify situations in which
     * a malicious application may have covered up part of its content for the purpose
     * of misleading the user or hijacking touches.  An appropriate response might be
     * to drop the suspect touches or to take additional precautions to confirm the user's
     * actual intent.
     */
    AMOTION_EVENT_FLAG_WINDOW_IS_PARTIALLY_OBSCURED = 0x2,

    /**
     * This flag indicates that the event has been generated by a gesture generator. It
     * provides a hint to the GestureDetector to not apply any touch slop.
     */
    AMOTION_EVENT_FLAG_IS_GENERATED_GESTURE = 0x8,

    /**
     * This flag indicates that the event will not cause a focus change if it is directed to an
     * unfocused window, even if it an ACTION_DOWN. This is typically used with pointer
     * gestures to allow the user to direct gestures to an unfocused window without bringing it
     * into focus.
     */
    AMOTION_EVENT_FLAG_NO_FOCUS_CHANGE = 0x40,

#if defined(__linux__)
    /**
     * This event was generated or modified by accessibility service.
     */
    AMOTION_EVENT_FLAG_IS_ACCESSIBILITY_EVENT =
            android::os::IInputConstants::INPUT_EVENT_FLAG_IS_ACCESSIBILITY_EVENT, // 0x800,
#else
    AMOTION_EVENT_FLAG_IS_ACCESSIBILITY_EVENT = 0x800,
#endif

    /* Motion event is inconsistent with previously sent motion events. */
    AMOTION_EVENT_FLAG_TAINTED = 0x80000000,
};

/**
 * Allowed VerifiedKeyEvent flags. All other flags from KeyEvent do not get verified.
 * These values must be kept in sync with VerifiedKeyEvent.java
 */
constexpr int32_t VERIFIED_KEY_EVENT_FLAGS =
        AKEY_EVENT_FLAG_CANCELED | AKEY_EVENT_FLAG_IS_ACCESSIBILITY_EVENT;

/**
 * Allowed VerifiedMotionEventFlags. All other flags from MotionEvent do not get verified.
 * These values must be kept in sync with VerifiedMotionEvent.java
 */
constexpr int32_t VERIFIED_MOTION_EVENT_FLAGS = AMOTION_EVENT_FLAG_WINDOW_IS_OBSCURED |
        AMOTION_EVENT_FLAG_WINDOW_IS_PARTIALLY_OBSCURED | AMOTION_EVENT_FLAG_IS_ACCESSIBILITY_EVENT;

/**
 * This flag indicates that the point up event has been canceled.
 * Typically this is used for palm event when the user has accidental touches.
 * TODO: Adjust flag to public api
 */
constexpr int32_t AMOTION_EVENT_FLAG_CANCELED = 0x20;

enum {
    /*
     * Indicates that an input device has switches.
     * This input source flag is hidden from the API because switches are only used by the system
     * and applications have no way to interact with them.
     */
    AINPUT_SOURCE_SWITCH = 0x80000000,
};

enum {
    /**
     * Constants for LEDs. Hidden from the API since we don't actually expose a way to interact
     * with LEDs to developers
     *
     * NOTE: If you add LEDs here, you must also add them to InputEventLabels.h
     */

    ALED_NUM_LOCK = 0x00,
    ALED_CAPS_LOCK = 0x01,
    ALED_SCROLL_LOCK = 0x02,
    ALED_COMPOSE = 0x03,
    ALED_KANA = 0x04,
    ALED_SLEEP = 0x05,
    ALED_SUSPEND = 0x06,
    ALED_MUTE = 0x07,
    ALED_MISC = 0x08,
    ALED_MAIL = 0x09,
    ALED_CHARGING = 0x0a,
    ALED_CONTROLLER_1 = 0x10,
    ALED_CONTROLLER_2 = 0x11,
    ALED_CONTROLLER_3 = 0x12,
    ALED_CONTROLLER_4 = 0x13,
};

/* Maximum number of controller LEDs we support */
#define MAX_CONTROLLER_LEDS 4

/*
 * Maximum number of pointers supported per motion event.
 * Smallest number of pointers is 1.
 * (We want at least 10 but some touch controllers obstensibly configured for 10 pointers
 * will occasionally emit 11.  There is not much harm making this constant bigger.)
 */
static constexpr size_t MAX_POINTERS = 16;

/*
 * Maximum number of samples supported per motion event.
 */
#define MAX_SAMPLES UINT16_MAX

/*
 * Maximum pointer id value supported in a motion event.
 * Smallest pointer id is 0.
 * (This is limited by our use of BitSet32 to track pointer assignments.)
 */
#define MAX_POINTER_ID 31

/*
 * Declare a concrete type for the NDK's input event forward declaration.
 */
struct AInputEvent {
    virtual ~AInputEvent() { }
};

/*
 * Declare a concrete type for the NDK's input device forward declaration.
 */
struct AInputDevice {
    virtual ~AInputDevice() { }
};


namespace android {

#ifdef __linux__
class Parcel;
#endif

/*
 * Apply the given transform to the point without applying any translation/offset.
 */
vec2 transformWithoutTranslation(const ui::Transform& transform, const vec2& xy);

const char* inputEventTypeToString(int32_t type);

std::string inputEventSourceToString(int32_t source);

bool isFromSource(uint32_t source, uint32_t test);

/*
 * Flags that flow alongside events in the input dispatch system to help with certain
 * policy decisions such as waking from device sleep.
 *
 * These flags are also defined in frameworks/base/core/java/android/view/WindowManagerPolicy.java.
 */
enum {
    /* These flags originate in RawEvents and are generally set in the key map.
     * NOTE: If you want a flag to be able to set in a keylayout file, then you must add it to
     * InputEventLabels.h as well. */

    // Indicates that the event should wake the device.
    POLICY_FLAG_WAKE = 0x00000001,

    // Indicates that the key is virtual, such as a capacitive button, and should
    // generate haptic feedback.  Virtual keys may be suppressed for some time
    // after a recent touch to prevent accidental activation of virtual keys adjacent
    // to the touch screen during an edge swipe.
    POLICY_FLAG_VIRTUAL = 0x00000002,

    // Indicates that the key is the special function modifier.
    POLICY_FLAG_FUNCTION = 0x00000004,

    // Indicates that the key represents a special gesture that has been detected by
    // the touch firmware or driver.  Causes touch events from the same device to be canceled.
    POLICY_FLAG_GESTURE = 0x00000008,

    POLICY_FLAG_RAW_MASK = 0x0000ffff,

#ifdef __linux__
    POLICY_FLAG_INJECTED_FROM_ACCESSIBILITY =
            android::os::IInputConstants::POLICY_FLAG_INJECTED_FROM_ACCESSIBILITY,
#else
    POLICY_FLAG_INJECTED_FROM_ACCESSIBILITY = 0x20000,
#endif

    /* These flags are set by the input dispatcher. */

    // Indicates that the input event was injected.
    POLICY_FLAG_INJECTED = 0x01000000,

    // Indicates that the input event is from a trusted source such as a directly attached
    // input device or an application with system-wide event injection permission.
    POLICY_FLAG_TRUSTED = 0x02000000,

    // Indicates that the input event has passed through an input filter.
    POLICY_FLAG_FILTERED = 0x04000000,

    // Disables automatic key repeating behavior.
    POLICY_FLAG_DISABLE_KEY_REPEAT = 0x08000000,

    /* These flags are set by the input reader policy as it intercepts each event. */

    // Indicates that the device was in an interactive state when the
    // event was intercepted.
    POLICY_FLAG_INTERACTIVE = 0x20000000,

    // Indicates that the event should be dispatched to applications.
    // The input event should still be sent to the InputDispatcher so that it can see all
    // input events received include those that it will not deliver.
    POLICY_FLAG_PASS_TO_USER = 0x40000000,

    POLICY_FLAG_POKE_USER_ACTIVIITY = 0x00100000,

};

/**
 * Classifications of the current gesture, if available.
 */
enum class MotionClassification : uint8_t {
    /**
     * No classification is available.
     */
    NONE = AMOTION_EVENT_CLASSIFICATION_NONE,
    /**
     * Too early to classify the current gesture. Need more events. Look for changes in the
     * upcoming motion events.
     */
    AMBIGUOUS_GESTURE = AMOTION_EVENT_CLASSIFICATION_AMBIGUOUS_GESTURE,
    /**
     * The current gesture likely represents a user intentionally exerting force on the touchscreen.
     */
    DEEP_PRESS = AMOTION_EVENT_CLASSIFICATION_DEEP_PRESS,
};

/**
 * String representation of MotionClassification
 */
const char* motionClassificationToString(MotionClassification classification);

/**
 * Portion of FrameMetrics timeline of interest to input code.
 */
enum GraphicsTimeline : size_t {
    /** Time when the app sent the buffer to SurfaceFlinger. */
    GPU_COMPLETED_TIME = 0,

    /** Time when the frame was presented on the display */
    PRESENT_TIME = 1,

    /** Total size of the 'GraphicsTimeline' array. Must always be last. */
    SIZE = 2
};

/**
 * Generator of unique numbers used to identify input events.
 *
 * Layout of ID:
 *     |--------------------------|---------------------------|
 *     |   2 bits for source      | 30 bits for random number |
 *     |--------------------------|---------------------------|
 */
class IdGenerator {
private:
    static constexpr uint32_t SOURCE_SHIFT = 30;

public:
    // Used to divide integer space to ensure no conflict among these sources./
    enum class Source : int32_t {
        INPUT_READER = static_cast<int32_t>(0x0u << SOURCE_SHIFT),
        INPUT_DISPATCHER = static_cast<int32_t>(0x1u << SOURCE_SHIFT),
        OTHER = static_cast<int32_t>(0x3u << SOURCE_SHIFT), // E.g. app injected events
    };
    IdGenerator(Source source);

    int32_t nextId() const;

    // Extract source from given id.
    static inline Source getSource(int32_t id) { return static_cast<Source>(SOURCE_MASK & id); }

private:
    const Source mSource;

    static constexpr int32_t SOURCE_MASK = static_cast<int32_t>(0x3u << SOURCE_SHIFT);
};

/**
 * Invalid value for cursor position. Used for non-mouse events, tests and injected events. Don't
 * use it for direct comparison with any other value, because NaN isn't equal to itself according to
 * IEEE 754. Use isnan() instead to check if a cursor position is valid.
 */
constexpr float AMOTION_EVENT_INVALID_CURSOR_POSITION = std::numeric_limits<float>::quiet_NaN();

/*
 * Pointer coordinate data.
 */
struct PointerCoords {
    enum { MAX_AXES = 30 }; // 30 so that sizeof(PointerCoords) == 128

    // Bitfield of axes that are present in this structure.
    uint64_t bits __attribute__((aligned(8)));

    // Values of axes that are stored in this structure packed in order by axis id
    // for each axis that is present in the structure according to 'bits'.
    float values[MAX_AXES];

    inline void clear() {
        BitSet64::clear(bits);
    }

    bool isEmpty() const {
        return BitSet64::isEmpty(bits);
    }

    float getAxisValue(int32_t axis) const;
    status_t setAxisValue(int32_t axis, float value);

    // Scale the pointer coordinates according to a global scale and a
    // window scale. The global scale will be applied to TOUCH/TOOL_MAJOR/MINOR
    // axes, however the window scaling will not.
    void scale(float globalScale, float windowXScale, float windowYScale);

    void transform(const ui::Transform& transform);

    inline float getX() const {
        return getAxisValue(AMOTION_EVENT_AXIS_X);
    }

    inline float getY() const {
        return getAxisValue(AMOTION_EVENT_AXIS_Y);
    }

    vec2 getXYValue() const { return vec2(getX(), getY()); }

#ifdef __linux__
    status_t readFromParcel(Parcel* parcel);
    status_t writeToParcel(Parcel* parcel) const;
#endif

    bool operator==(const PointerCoords& other) const;
    inline bool operator!=(const PointerCoords& other) const {
        return !(*this == other);
    }

    void copyFrom(const PointerCoords& other);

private:
    void tooManyAxes(int axis);
};

/*
 * Pointer property data.
 */
struct PointerProperties {
    // The id of the pointer.
    int32_t id;

    // The pointer tool type.
    int32_t toolType;

    inline void clear() {
        id = -1;
        toolType = 0;
    }

    bool operator==(const PointerProperties& other) const;
    inline bool operator!=(const PointerProperties& other) const {
        return !(*this == other);
    }

    void copyFrom(const PointerProperties& other);
};

/*
 * Input events.
 */
class InputEvent : public AInputEvent {
public:
    virtual ~InputEvent() { }

    virtual int32_t getType() const = 0;

    inline int32_t getId() const { return mId; }

    inline int32_t getDeviceId() const { return mDeviceId; }

    inline uint32_t getSource() const { return mSource; }

    inline void setSource(uint32_t source) { mSource = source; }

    inline int32_t getDisplayId() const { return mDisplayId; }

    inline void setDisplayId(int32_t displayId) { mDisplayId = displayId; }

    inline std::array<uint8_t, 32> getHmac() const { return mHmac; }

    static int32_t nextId();

protected:
    void initialize(int32_t id, int32_t deviceId, uint32_t source, int32_t displayId,
                    std::array<uint8_t, 32> hmac);

    void initialize(const InputEvent& from);

    int32_t mId;
    int32_t mDeviceId;
    uint32_t mSource;
    int32_t mDisplayId;
    std::array<uint8_t, 32> mHmac;
};

/*
 * Key events.
 */
class KeyEvent : public InputEvent {
public:
    virtual ~KeyEvent() { }

    virtual int32_t getType() const { return AINPUT_EVENT_TYPE_KEY; }

    inline int32_t getAction() const { return mAction; }

    inline int32_t getFlags() const { return mFlags; }

    inline void setFlags(int32_t flags) { mFlags = flags; }

    inline int32_t getKeyCode() const { return mKeyCode; }

    inline int32_t getScanCode() const { return mScanCode; }

    inline int32_t getMetaState() const { return mMetaState; }

    inline int32_t getRepeatCount() const { return mRepeatCount; }

    inline nsecs_t getDownTime() const { return mDownTime; }

    inline nsecs_t getEventTime() const { return mEventTime; }

    static const char* getLabel(int32_t keyCode);
    static int32_t getKeyCodeFromLabel(const char* label);

    void initialize(int32_t id, int32_t deviceId, uint32_t source, int32_t displayId,
                    std::array<uint8_t, 32> hmac, int32_t action, int32_t flags, int32_t keyCode,
                    int32_t scanCode, int32_t metaState, int32_t repeatCount, nsecs_t downTime,
                    nsecs_t eventTime);
    void initialize(const KeyEvent& from);

    static const char* actionToString(int32_t action);

protected:
    int32_t mAction;
    int32_t mFlags;
    int32_t mKeyCode;
    int32_t mScanCode;
    int32_t mMetaState;
    int32_t mRepeatCount;
    nsecs_t mDownTime;
    nsecs_t mEventTime;
};

/*
 * Motion events.
 */
class MotionEvent : public InputEvent {
public:
    virtual ~MotionEvent() { }

    virtual int32_t getType() const { return AINPUT_EVENT_TYPE_MOTION; }

    inline int32_t getAction() const { return mAction; }

    static int32_t getActionMasked(int32_t action) { return action & AMOTION_EVENT_ACTION_MASK; }

    inline int32_t getActionMasked() const { return getActionMasked(mAction); }

    static uint8_t getActionIndex(int32_t action) {
        return (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    }

    inline int32_t getActionIndex() const { return getActionIndex(mAction); }

    inline void setAction(int32_t action) { mAction = action; }

    inline int32_t getFlags() const { return mFlags; }

    inline void setFlags(int32_t flags) { mFlags = flags; }

    inline int32_t getEdgeFlags() const { return mEdgeFlags; }

    inline void setEdgeFlags(int32_t edgeFlags) { mEdgeFlags = edgeFlags; }

    inline int32_t getMetaState() const { return mMetaState; }

    inline void setMetaState(int32_t metaState) { mMetaState = metaState; }

    inline int32_t getButtonState() const { return mButtonState; }

    inline void setButtonState(int32_t buttonState) { mButtonState = buttonState; }

    inline MotionClassification getClassification() const { return mClassification; }

    inline int32_t getActionButton() const { return mActionButton; }

    inline void setActionButton(int32_t button) { mActionButton = button; }

    inline float getXOffset() const { return mTransform.tx(); }

    inline float getYOffset() const { return mTransform.ty(); }

    inline const ui::Transform& getTransform() const { return mTransform; }

    int getSurfaceRotation() const;

    inline float getXPrecision() const { return mXPrecision; }

    inline float getYPrecision() const { return mYPrecision; }

    inline float getRawXCursorPosition() const { return mRawXCursorPosition; }

    float getXCursorPosition() const;

    inline float getRawYCursorPosition() const { return mRawYCursorPosition; }

    float getYCursorPosition() const;

    void setCursorPosition(float x, float y);

    inline const ui::Transform& getRawTransform() const { return mRawTransform; }

    static inline bool isValidCursorPosition(float x, float y) { return !isnan(x) && !isnan(y); }

    inline nsecs_t getDownTime() const { return mDownTime; }

    inline void setDownTime(nsecs_t downTime) { mDownTime = downTime; }

    inline size_t getPointerCount() const { return mPointerProperties.size(); }

    inline const PointerProperties* getPointerProperties(size_t pointerIndex) const {
        return &mPointerProperties[pointerIndex];
    }

    inline int32_t getPointerId(size_t pointerIndex) const {
        return mPointerProperties[pointerIndex].id;
    }

    inline int32_t getToolType(size_t pointerIndex) const {
        return mPointerProperties[pointerIndex].toolType;
    }

    inline nsecs_t getEventTime() const { return mSampleEventTimes[getHistorySize()]; }

    /**
     * The actual raw pointer coords: whatever comes from the input device without any external
     * transforms applied.
     */
    const PointerCoords* getRawPointerCoords(size_t pointerIndex) const;

    /**
     * This is the raw axis value. However, for X/Y axes, this currently applies a "compat-raw"
     * transform because many apps (incorrectly) assumed that raw == oriented-screen-space.
     * "compat raw" is raw coordinates with screen rotation applied.
     */
    float getRawAxisValue(int32_t axis, size_t pointerIndex) const;

    inline float getRawX(size_t pointerIndex) const {
        return getRawAxisValue(AMOTION_EVENT_AXIS_X, pointerIndex);
    }

    inline float getRawY(size_t pointerIndex) const {
        return getRawAxisValue(AMOTION_EVENT_AXIS_Y, pointerIndex);
    }

    float getAxisValue(int32_t axis, size_t pointerIndex) const;

    /**
     * Get the X coordinate of the latest sample in this MotionEvent for pointer 'pointerIndex'.
     * Identical to calling getHistoricalX(pointerIndex, getHistorySize()).
     */
    inline float getX(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_X, pointerIndex);
    }

    /**
     * Get the Y coordinate of the latest sample in this MotionEvent for pointer 'pointerIndex'.
     * Identical to calling getHistoricalX(pointerIndex, getHistorySize()).
     */
    inline float getY(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_Y, pointerIndex);
    }

    inline float getPressure(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_PRESSURE, pointerIndex);
    }

    inline float getSize(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_SIZE, pointerIndex);
    }

    inline float getTouchMajor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, pointerIndex);
    }

    inline float getTouchMinor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, pointerIndex);
    }

    inline float getToolMajor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOOL_MAJOR, pointerIndex);
    }

    inline float getToolMinor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOOL_MINOR, pointerIndex);
    }

    inline float getOrientation(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, pointerIndex);
    }

    inline size_t getHistorySize() const { return mSampleEventTimes.size() - 1; }

    inline nsecs_t getHistoricalEventTime(size_t historicalIndex) const {
        return mSampleEventTimes[historicalIndex];
    }

    /**
     * The actual raw pointer coords: whatever comes from the input device without any external
     * transforms applied.
     */
    const PointerCoords* getHistoricalRawPointerCoords(
            size_t pointerIndex, size_t historicalIndex) const;

    /**
     * This is the raw axis value. However, for X/Y axes, this currently applies a "compat-raw"
     * transform because many apps (incorrectly) assumed that raw == oriented-screen-space.
     * "compat raw" is raw coordinates with screen rotation applied.
     */
    float getHistoricalRawAxisValue(int32_t axis, size_t pointerIndex,
            size_t historicalIndex) const;

    inline float getHistoricalRawX(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalRawAxisValue(
                AMOTION_EVENT_AXIS_X, pointerIndex, historicalIndex);
    }

    inline float getHistoricalRawY(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalRawAxisValue(
                AMOTION_EVENT_AXIS_Y, pointerIndex, historicalIndex);
    }

    float getHistoricalAxisValue(int32_t axis, size_t pointerIndex, size_t historicalIndex) const;

    inline float getHistoricalX(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_X, pointerIndex, historicalIndex);
    }

    inline float getHistoricalY(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_Y, pointerIndex, historicalIndex);
    }

    inline float getHistoricalPressure(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_PRESSURE, pointerIndex, historicalIndex);
    }

    inline float getHistoricalSize(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_SIZE, pointerIndex, historicalIndex);
    }

    inline float getHistoricalTouchMajor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOUCH_MAJOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalTouchMinor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOUCH_MINOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalToolMajor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOOL_MAJOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalToolMinor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOOL_MINOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalOrientation(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_ORIENTATION, pointerIndex, historicalIndex);
    }

    ssize_t findPointerIndex(int32_t pointerId) const;

    void initialize(int32_t id, int32_t deviceId, uint32_t source, int32_t displayId,
                    std::array<uint8_t, 32> hmac, int32_t action, int32_t actionButton,
                    int32_t flags, int32_t edgeFlags, int32_t metaState, int32_t buttonState,
                    MotionClassification classification, const ui::Transform& transform,
                    float xPrecision, float yPrecision, float rawXCursorPosition,
                    float rawYCursorPosition, const ui::Transform& rawTransform, nsecs_t downTime,
                    nsecs_t eventTime, size_t pointerCount,
                    const PointerProperties* pointerProperties, const PointerCoords* pointerCoords);

    void copyFrom(const MotionEvent* other, bool keepHistory);

    void addSample(
            nsecs_t eventTime,
            const PointerCoords* pointerCoords);

    void offsetLocation(float xOffset, float yOffset);

    void scale(float globalScaleFactor);

    // Set 3x3 perspective matrix transformation.
    // Matrix is in row-major form and compatible with SkMatrix.
    void transform(const std::array<float, 9>& matrix);

    // Apply 3x3 perspective matrix transformation only to content (do not modify mTransform).
    // Matrix is in row-major form and compatible with SkMatrix.
    void applyTransform(const std::array<float, 9>& matrix);

#ifdef __linux__
    status_t readFromParcel(Parcel* parcel);
    status_t writeToParcel(Parcel* parcel) const;
#endif

    static bool isTouchEvent(uint32_t source, int32_t action);
    inline bool isTouchEvent() const {
        return isTouchEvent(mSource, mAction);
    }

    // Low-level accessors.
    inline const PointerProperties* getPointerProperties() const {
        return mPointerProperties.data();
    }
    inline const nsecs_t* getSampleEventTimes() const { return mSampleEventTimes.data(); }
    inline const PointerCoords* getSamplePointerCoords() const {
        return mSamplePointerCoords.data();
    }

    static const char* getLabel(int32_t axis);
    static int32_t getAxisFromLabel(const char* label);

    static std::string actionToString(int32_t action);

    // MotionEvent will transform various axes in different ways, based on the source. For
    // example, the x and y axes will not have any offsets/translations applied if it comes from a
    // relative mouse device (since SOURCE_RELATIVE_MOUSE is a non-pointer source). These methods
    // are used to apply these transformations for different axes.
    static vec2 calculateTransformedXY(uint32_t source, const ui::Transform&, const vec2& xy);
    static float calculateTransformedAxisValue(int32_t axis, uint32_t source, const ui::Transform&,
                                               const PointerCoords&);
    static PointerCoords calculateTransformedCoords(uint32_t source, const ui::Transform&,
                                                    const PointerCoords&);

protected:
    int32_t mAction;
    int32_t mActionButton;
    int32_t mFlags;
    int32_t mEdgeFlags;
    int32_t mMetaState;
    int32_t mButtonState;
    MotionClassification mClassification;
    ui::Transform mTransform;
    float mXPrecision;
    float mYPrecision;
    float mRawXCursorPosition;
    float mRawYCursorPosition;
    ui::Transform mRawTransform;
    nsecs_t mDownTime;
    std::vector<PointerProperties> mPointerProperties;
    std::vector<nsecs_t> mSampleEventTimes;
    std::vector<PointerCoords> mSamplePointerCoords;
};

std::ostream& operator<<(std::ostream& out, const MotionEvent& event);

/*
 * Focus events.
 */
class FocusEvent : public InputEvent {
public:
    virtual ~FocusEvent() {}

    virtual int32_t getType() const override { return AINPUT_EVENT_TYPE_FOCUS; }

    inline bool getHasFocus() const { return mHasFocus; }

    void initialize(int32_t id, bool hasFocus);

    void initialize(const FocusEvent& from);

protected:
    bool mHasFocus;
};

/*
 * Capture events.
 */
class CaptureEvent : public InputEvent {
public:
    virtual ~CaptureEvent() {}

    virtual int32_t getType() const override { return AINPUT_EVENT_TYPE_CAPTURE; }

    inline bool getPointerCaptureEnabled() const { return mPointerCaptureEnabled; }

    void initialize(int32_t id, bool pointerCaptureEnabled);

    void initialize(const CaptureEvent& from);

protected:
    bool mPointerCaptureEnabled;
};

/*
 * Drag events.
 */
class DragEvent : public InputEvent {
public:
    virtual ~DragEvent() {}

    virtual int32_t getType() const override { return AINPUT_EVENT_TYPE_DRAG; }

    inline bool isExiting() const { return mIsExiting; }

    inline float getX() const { return mX; }

    inline float getY() const { return mY; }

    void initialize(int32_t id, float x, float y, bool isExiting);

    void initialize(const DragEvent& from);

protected:
    bool mIsExiting;
    float mX, mY;
};

/*
 * Touch mode events.
 */
class TouchModeEvent : public InputEvent {
public:
    virtual ~TouchModeEvent() {}

    virtual int32_t getType() const override { return AINPUT_EVENT_TYPE_TOUCH_MODE; }

    inline bool isInTouchMode() const { return mIsInTouchMode; }

    void initialize(int32_t id, bool isInTouchMode);

    void initialize(const TouchModeEvent& from);

protected:
    bool mIsInTouchMode;
};

/**
 * Base class for verified events.
 * Do not create a VerifiedInputEvent explicitly.
 * Use helper functions to create them from InputEvents.
 */
struct __attribute__((__packed__)) VerifiedInputEvent {
    enum class Type : int32_t {
        KEY = AINPUT_EVENT_TYPE_KEY,
        MOTION = AINPUT_EVENT_TYPE_MOTION,
    };

    Type type;
    int32_t deviceId;
    nsecs_t eventTimeNanos;
    uint32_t source;
    int32_t displayId;
};

/**
 * Same as KeyEvent, but only contains the data that can be verified.
 * If you update this class, you must also update VerifiedKeyEvent.java
 */
struct __attribute__((__packed__)) VerifiedKeyEvent : public VerifiedInputEvent {
    int32_t action;
    int32_t flags;
    nsecs_t downTimeNanos;
    int32_t keyCode;
    int32_t scanCode;
    int32_t metaState;
    int32_t repeatCount;
};

/**
 * Same as MotionEvent, but only contains the data that can be verified.
 * If you update this class, you must also update VerifiedMotionEvent.java
 */
struct __attribute__((__packed__)) VerifiedMotionEvent : public VerifiedInputEvent {
    float rawX;
    float rawY;
    int32_t actionMasked;
    int32_t flags;
    nsecs_t downTimeNanos;
    int32_t metaState;
    int32_t buttonState;
};

VerifiedKeyEvent verifiedKeyEventFromKeyEvent(const KeyEvent& event);
VerifiedMotionEvent verifiedMotionEventFromMotionEvent(const MotionEvent& event);

/*
 * Input event factory.
 */
class InputEventFactoryInterface {
protected:
    virtual ~InputEventFactoryInterface() { }

public:
    InputEventFactoryInterface() { }

    virtual KeyEvent* createKeyEvent() = 0;
    virtual MotionEvent* createMotionEvent() = 0;
    virtual FocusEvent* createFocusEvent() = 0;
    virtual CaptureEvent* createCaptureEvent() = 0;
    virtual DragEvent* createDragEvent() = 0;
    virtual TouchModeEvent* createTouchModeEvent() = 0;
};

/*
 * A simple input event factory implementation that uses a single preallocated instance
 * of each type of input event that are reused for each request.
 */
class PreallocatedInputEventFactory : public InputEventFactoryInterface {
public:
    PreallocatedInputEventFactory() { }
    virtual ~PreallocatedInputEventFactory() { }

    virtual KeyEvent* createKeyEvent() override { return &mKeyEvent; }
    virtual MotionEvent* createMotionEvent() override { return &mMotionEvent; }
    virtual FocusEvent* createFocusEvent() override { return &mFocusEvent; }
    virtual CaptureEvent* createCaptureEvent() override { return &mCaptureEvent; }
    virtual DragEvent* createDragEvent() override { return &mDragEvent; }
    virtual TouchModeEvent* createTouchModeEvent() override { return &mTouchModeEvent; }

private:
    KeyEvent mKeyEvent;
    MotionEvent mMotionEvent;
    FocusEvent mFocusEvent;
    CaptureEvent mCaptureEvent;
    DragEvent mDragEvent;
    TouchModeEvent mTouchModeEvent;
};

/*
 * An input event factory implementation that maintains a pool of input events.
 */
class PooledInputEventFactory : public InputEventFactoryInterface {
public:
    explicit PooledInputEventFactory(size_t maxPoolSize = 20);
    virtual ~PooledInputEventFactory();

    virtual KeyEvent* createKeyEvent() override;
    virtual MotionEvent* createMotionEvent() override;
    virtual FocusEvent* createFocusEvent() override;
    virtual CaptureEvent* createCaptureEvent() override;
    virtual DragEvent* createDragEvent() override;
    virtual TouchModeEvent* createTouchModeEvent() override;

    void recycle(InputEvent* event);

private:
    const size_t mMaxPoolSize;

    std::queue<std::unique_ptr<KeyEvent>> mKeyEventPool;
    std::queue<std::unique_ptr<MotionEvent>> mMotionEventPool;
    std::queue<std::unique_ptr<FocusEvent>> mFocusEventPool;
    std::queue<std::unique_ptr<CaptureEvent>> mCaptureEventPool;
    std::queue<std::unique_ptr<DragEvent>> mDragEventPool;
    std::queue<std::unique_ptr<TouchModeEvent>> mTouchModeEventPool;
};

/*
 * Describes a unique request to enable or disable Pointer Capture.
 */
struct PointerCaptureRequest {
public:
    inline PointerCaptureRequest() : enable(false), seq(0) {}
    inline PointerCaptureRequest(bool enable, uint32_t seq) : enable(enable), seq(seq) {}
    inline bool operator==(const PointerCaptureRequest& other) const {
        return enable == other.enable && seq == other.seq;
    }
    explicit inline operator bool() const { return enable; }

    // True iff this is a request to enable Pointer Capture.
    bool enable;

    // The sequence number for the request.
    uint32_t seq;
};

} // namespace android

#endif // _LIBINPUT_INPUT_H

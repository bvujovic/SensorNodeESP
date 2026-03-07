#pragma once
#include <Arduino.h>

class Retryer
{
public:
    enum class State
    {
        Idle,
        ReadyToSend,
        WaitingForResponse,
        Success,
        Failed
    };

    Retryer(uint8_t maxRetries,
            unsigned long retryDelayMs,
            unsigned long responseTimeoutMs)
        : _maxRetries(maxRetries),
          _retryDelay(retryDelayMs),
          _responseTimeout(responseTimeoutMs)
    {
        reset();
    }

    void start()
    {
        reset();
        _state = State::ReadyToSend;
    }

    void update();

    // Call after esp_now_send()
    void onSendResult(bool success);

    // Call from OnDataRecv()
    void onResponseReceived()
    {
        if (_state == State::WaitingForResponse)
            _state = State::Success;
    }

    bool readyToSend() const { return _state == State::ReadyToSend; }

    // Returns true if exchange ended with either success or failure
    bool stateEnd() const { return _state == State::Success || _state == State::Failed; }

    State state() const { return _state; }

    uint8_t attempts() const { return _attempt; }

    void reset()
    {
        _attempt = 0;
        _state = State::Idle;
        _lastRetryTime = 0;
        _responseStart = 0;
    }

private:
    State _state = State::Idle;
    void scheduleRetry();

    uint8_t _maxRetries;
    uint8_t _attempt = 0;

    unsigned long _retryDelay;
    unsigned long _responseTimeout;

    unsigned long _lastRetryTime = 0;
    unsigned long _responseStart = 0;
};

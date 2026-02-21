#include "Retryer.h"

void Retryer::update()
{
    unsigned long now = millis();

    switch (_state)
    {
    case State::WaitingForResponse:
        if (now - _responseStart >= _responseTimeout)
            scheduleRetry();
        break;
    case State::Idle:
    case State::Sending:
    case State::Success:
    case State::Failed:
    default:
        break;
    }

    if (_state == State::Idle && _attempt < _maxRetries && now - _lastRetryTime >= _retryDelay)
    {
        _state = State::Sending;
    }
}

void Retryer::onSendResult(bool success)
{
    if (_state != State::Sending)
        return;

    if (!success)
        scheduleRetry();
    else
    {
        _state = State::WaitingForResponse;
        _responseStart = millis();
    }
}

void Retryer::scheduleRetry()
{
    _attempt++;
    if (_attempt >= _maxRetries)
    {
        _state = State::Failed;
        return;
    }
    _lastRetryTime = millis();
    _state = State::Idle;
}
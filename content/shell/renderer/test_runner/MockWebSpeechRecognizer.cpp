// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/renderer/test_runner/MockWebSpeechRecognizer.h"

#include "base/logging.h"
#include "content/shell/renderer/test_runner/WebTestDelegate.h"
#include "third_party/WebKit/public/web/WebSpeechRecognitionResult.h"
#include "third_party/WebKit/public/web/WebSpeechRecognizerClient.h"

using namespace blink;

namespace content {

namespace {

// Task class for calling a client function that does not take any parameters.
typedef void (WebSpeechRecognizerClient::*ClientFunctionPointer)(const WebSpeechRecognitionHandle&);
class ClientCallTask : public MockWebSpeechRecognizer::Task {
public:
    ClientCallTask(MockWebSpeechRecognizer* mock, ClientFunctionPointer function)
        : MockWebSpeechRecognizer::Task(mock)
        , m_function(function)
    {
    }

    virtual void run() OVERRIDE { (m_recognizer->client()->*m_function)(m_recognizer->handle()); }

private:
    ClientFunctionPointer m_function;
};

// Task for delivering a result event.
class ResultTask : public MockWebSpeechRecognizer::Task {
public:
    ResultTask(MockWebSpeechRecognizer* mock, const WebString transcript, float confidence)
        : MockWebSpeechRecognizer::Task(mock)
        , m_transcript(transcript)
        , m_confidence(confidence)
    {
    }

    virtual void run() OVERRIDE
    {
        WebVector<WebString> transcripts(static_cast<size_t>(1));
        WebVector<float> confidences(static_cast<size_t>(1));
        transcripts[0] = m_transcript;
        confidences[0] = m_confidence;
        WebVector<WebSpeechRecognitionResult> finalResults(static_cast<size_t>(1));
        WebVector<WebSpeechRecognitionResult> interimResults;
        finalResults[0].assign(transcripts, confidences, true);

        m_recognizer->client()->didReceiveResults(m_recognizer->handle(), finalResults, interimResults);
    }

private:
    WebString m_transcript;
    float m_confidence;
};

// Task for delivering a nomatch event.
class NoMatchTask : public MockWebSpeechRecognizer::Task {
public:
    NoMatchTask(MockWebSpeechRecognizer* mock) : MockWebSpeechRecognizer::Task(mock) { }
    virtual void run() OVERRIDE { m_recognizer->client()->didReceiveNoMatch(m_recognizer->handle(), WebSpeechRecognitionResult()); }
};

// Task for delivering an error event.
class ErrorTask : public MockWebSpeechRecognizer::Task {
public:
    ErrorTask(MockWebSpeechRecognizer* mock, WebSpeechRecognizerClient::ErrorCode code, const WebString& message)
        : MockWebSpeechRecognizer::Task(mock)
        , m_code(code)
        , m_message(message)
    {
    }

    virtual void run() OVERRIDE { m_recognizer->client()->didReceiveError(m_recognizer->handle(), m_message, m_code); }

private:
    WebSpeechRecognizerClient::ErrorCode m_code;
    WebString m_message;
};

} // namespace

MockWebSpeechRecognizer::MockWebSpeechRecognizer()
    : m_wasAborted(false)
    , m_taskQueueRunning(false)
    , m_delegate(0)
{
}

MockWebSpeechRecognizer::~MockWebSpeechRecognizer()
{
    clearTaskQueue();
}

void MockWebSpeechRecognizer::setDelegate(WebTestDelegate* delegate)
{
    m_delegate = delegate;
}

void MockWebSpeechRecognizer::start(const WebSpeechRecognitionHandle& handle, const WebSpeechRecognitionParams& params, WebSpeechRecognizerClient* client)
{
    m_wasAborted = false;
    m_handle = handle;
    m_client = client;

    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didStart));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didStartAudio));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didStartSound));

    if (!m_mockTranscripts.empty()) {
        DCHECK_EQ(m_mockTranscripts.size(), m_mockConfidences.size());

        for (size_t i = 0; i < m_mockTranscripts.size(); ++i)
            m_taskQueue.push_back(new ResultTask(this, m_mockTranscripts[i], m_mockConfidences[i]));

        m_mockTranscripts.clear();
        m_mockConfidences.clear();
    } else
        m_taskQueue.push_back(new NoMatchTask(this));

    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEndSound));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEndAudio));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEnd));

    startTaskQueue();
}

void MockWebSpeechRecognizer::stop(const WebSpeechRecognitionHandle& handle, WebSpeechRecognizerClient* client)
{
    m_handle = handle;
    m_client = client;

    // FIXME: Implement.
    NOTREACHED();
}

void MockWebSpeechRecognizer::abort(const WebSpeechRecognitionHandle& handle, WebSpeechRecognizerClient* client)
{
    m_handle = handle;
    m_client = client;

    clearTaskQueue();
    m_wasAborted = true;
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEnd));
    startTaskQueue();
}

void MockWebSpeechRecognizer::addMockResult(const WebString& transcript, float confidence)
{
    m_mockTranscripts.push_back(transcript);
    m_mockConfidences.push_back(confidence);
}

void MockWebSpeechRecognizer::setError(const WebString& error, const WebString& message)
{
    WebSpeechRecognizerClient::ErrorCode code;
    if (error == "OtherError")
        code = WebSpeechRecognizerClient::OtherError;
    else if (error == "NoSpeechError")
        code = WebSpeechRecognizerClient::NoSpeechError;
    else if (error == "AbortedError")
        code = WebSpeechRecognizerClient::AbortedError;
    else if (error == "AudioCaptureError")
        code = WebSpeechRecognizerClient::AudioCaptureError;
    else if (error == "NetworkError")
        code = WebSpeechRecognizerClient::NetworkError;
    else if (error == "NotAllowedError")
        code = WebSpeechRecognizerClient::NotAllowedError;
    else if (error == "ServiceNotAllowedError")
        code = WebSpeechRecognizerClient::ServiceNotAllowedError;
    else if (error == "BadGrammarError")
        code = WebSpeechRecognizerClient::BadGrammarError;
    else if (error == "LanguageNotSupportedError")
        code = WebSpeechRecognizerClient::LanguageNotSupportedError;
    else
        return;

    clearTaskQueue();
    m_taskQueue.push_back(new ErrorTask(this, code, message));
    m_taskQueue.push_back(new ClientCallTask(this, &WebSpeechRecognizerClient::didEnd));
    startTaskQueue();
}

void MockWebSpeechRecognizer::startTaskQueue()
{
    if (m_taskQueueRunning)
        return;
    m_delegate->postTask(new StepTask(this));
    m_taskQueueRunning = true;
}

void MockWebSpeechRecognizer::clearTaskQueue()
{
    while (!m_taskQueue.empty()) {
        delete m_taskQueue.front();
        m_taskQueue.pop_front();
    }
    m_taskQueueRunning = false;
}

void MockWebSpeechRecognizer::StepTask::runIfValid()
{
    if (m_object->m_taskQueue.empty()) {
        m_object->m_taskQueueRunning = false;
        return;
    }

    Task* task = m_object->m_taskQueue.front();
    m_object->m_taskQueue.pop_front();
    task->run();
    delete task;

    if (m_object->m_taskQueue.empty()) {
        m_object->m_taskQueueRunning = false;
        return;
    }

    m_object->m_delegate->postTask(new StepTask(m_object));
}

}  // namespace content

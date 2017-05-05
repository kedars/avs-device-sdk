/*
 * DirectiveSequencer.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_SEQUENCER_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_SEQUENCER_H_

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include <AVSCommon/ExceptionEncounteredSenderInterface.h>

#include "ADSL/DirectiveProcessor.h"
#include "ADSL/DirectiveRouter.h"
#include "ADSL/DirectiveSequencerInterface.h"

namespace alexaClientSDK {
namespace adsl {

/**
 * Class for sequencing and handling a stream of @c AVSDirective instances.
 */
class DirectiveSequencer : public DirectiveSequencerInterface {
public:
    /**
     * Create a DirectiveSequencer.
     *
     * @param exceptionSender An instance of the @c ExceptionEncounteredSenderInterface used to send
     * ExceptionEncountered messages to AVS for directives that are not handled.
     * @return Returns a new DirectiveSequencer, or nullptr if the operation failed.
     */
    static std::unique_ptr<DirectiveSequencerInterface> create(
            std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender);

    ~DirectiveSequencer() override;

    bool addDirectiveHandlers(const DirectiveHandlerConfiguration& configuration) override;

    bool removeDirectiveHandlers(const DirectiveHandlerConfiguration& configuration) override;

    void setDialogRequestId(const std::string& dialogRequestId) override;

    bool onDirective(std::shared_ptr<avsCommon::AVSDirective> directive) override;

    void shutdown() override;

private:
    /**
     * Constructor.
     *
     * @param exceptionSender An instance of the @c ExceptionEncounteredSenderInterface used to send
     * ExceptionEncountered messages to AVS for directives that are not handled.
     */
    DirectiveSequencer(std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender);

    /**
     * Thread method for m_receivingThread.
     */
    void receivingLoop();

    /**
     * Process the next item in @c m_receivingQueue.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     *
     * @param lock A @c unique_lock on m_mutex, allowing this method to release the lock around callbacks
     * that need to be invoked.
     */
    void receiveDirectiveLocked(std::unique_lock<std::mutex>& lock);

    /// Serializes access to data members (besides m_directiveRouter and m_directiveProcessor).
    std::mutex m_mutex;

    /// The @c ExceptionEncounteredSenderInterface instance to send exceptions to.
    std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> m_exceptionSender;

    /// Whether or not the @c DirectiveReceiver is shutting down.
    bool m_isShuttingDown;

    /// Object used to route directives to their assigned handler.
    DirectiveRouter m_directiveRouter;

    /// Object used to drive handling of @c AVSDirectives.
    std::shared_ptr<DirectiveProcessor> m_directiveProcessor;

    /// Queue of @c AVSDirectives waiting to be received.
    std::deque<std::shared_ptr<avsCommon::AVSDirective>> m_receivingQueue;

    /// Condition variable used to wake m_receivingLoop when waiting.
    std::condition_variable m_wakeReceivingLoop;

    /// Thread to receive directives.
    std::thread m_receivingThread;
};

} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_SEQUENCER_H_
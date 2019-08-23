//------------------------------------------------------------------------------
// Copyright (c) 2004-2019 Darby Johnston
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions, and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions, and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the names of the copyright holders nor the names of any
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//------------------------------------------------------------------------------

#include <djvViewApp/Media.h>

#include <djvAV/AVSystem.h>

#include <djvCore/Context.h>
#include <djvCore/LogSystem.h>
#include <djvCore/Timer.h>

#include <RtAudio.h>

#include <condition_variable>
#include <queue>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        namespace
        {
            //! \todo Should this be configurable?
            const size_t bufferFrameCount = 256;
            const size_t videoQueueSize = 30;
            
        } // namespace

        struct Media::Private
        {
            Context * context = nullptr;

            Core::FileSystem::FileInfo fileInfo;
            std::shared_ptr<ValueSubject<AV::IO::Info> > info;
            AV::IO::VideoInfo videoInfo;
            AV::IO::AudioInfo audioInfo;
            std::shared_ptr<ValueSubject<bool> > reload;
            std::shared_ptr<ValueSubject<size_t> > layer;
            std::shared_ptr<ValueSubject<Time::Speed> > speed;
            std::shared_ptr<ValueSubject<Time::Speed> > defaultSpeed;
            std::shared_ptr<ValueSubject<float> > realSpeed;
            std::shared_ptr<ValueSubject<bool> > playEveryFrame;
            std::shared_ptr<ValueSubject<Frame::Sequence> > sequence;
            std::shared_ptr<ValueSubject<Frame::Number> > currentFrame;
            std::shared_ptr<ValueSubject<std::shared_ptr<AV::Image::Image> > > currentImage;
            std::shared_ptr<ValueSubject<Playback> > playback;
            std::shared_ptr<ValueSubject<PlaybackMode> > playbackMode;
            std::shared_ptr<ValueSubject<bool> > inOutPointsEnabled;
            std::shared_ptr<ValueSubject<Frame::Number> > inPoint;
            std::shared_ptr<ValueSubject<Frame::Number> > outPoint;
            std::shared_ptr<ValueSubject<float> > volume;
            std::shared_ptr<ValueSubject<bool> > mute;
            std::shared_ptr<ValueSubject<size_t> > threadCount;
            std::shared_ptr<ListSubject<Frame::Range> > cachedFrames;

            std::shared_ptr<ValueSubject<size_t> > videoQueueMax;
            std::shared_ptr<ValueSubject<size_t> > videoQueueCount;
            std::shared_ptr<ValueSubject<size_t> > audioQueueMax;
            std::shared_ptr<ValueSubject<size_t> > audioQueueCount;
            std::shared_ptr<AV::IO::IRead> read;

            AV::IO::Direction ioDirection = AV::IO::Direction::Forward;
            std::unique_ptr<RtAudio> rtAudio;
            std::shared_ptr<AV::Audio::Data> audioData;
            size_t audioDataOffset = 0;
            size_t audioDataTotal = 0;
            std::chrono::system_clock::time_point audioDataTimeOffset;
            Frame::Number frameOffset = 0;
            std::chrono::system_clock::time_point startTime;
            std::chrono::system_clock::time_point realSpeedTime;
            size_t realSpeedFrameCount = 0;
            std::shared_ptr<Time::Timer> queueTimer;
            std::shared_ptr<Time::Timer> playbackTimer;
            std::shared_ptr<Time::Timer> realSpeedTimer;
            std::shared_ptr<Time::Timer> cachedFramesTimer;
            std::shared_ptr<Time::Timer> debugTimer;
        };

        void Media::_init(const Core::FileSystem::FileInfo& fileInfo, Context * context)
        {
            DJV_PRIVATE_PTR();
            p.context = context;

            p.fileInfo = fileInfo;
            p.info = ValueSubject<AV::IO::Info>::create();
            p.reload = ValueSubject<bool>::create();
            p.layer = ValueSubject<size_t>::create();
            p.speed = ValueSubject<Time::Speed>::create();
            p.defaultSpeed = ValueSubject<Time::Speed>::create();
            p.realSpeed = ValueSubject<float>::create();
            p.playEveryFrame = ValueSubject<bool>::create();
            p.sequence = ValueSubject<Frame::Sequence>::create();
            p.currentFrame = ValueSubject<Frame::Number>::create();
            p.currentImage = ValueSubject<std::shared_ptr<AV::Image::Image> >::create();
            p.playback = ValueSubject<Playback>::create();
            p.playbackMode = ValueSubject<PlaybackMode>::create(PlaybackMode::Loop);
            p.inOutPointsEnabled = ValueSubject<bool>::create(false);
            p.inPoint = ValueSubject<Frame::Number>::create(Frame::invalid);
            p.outPoint = ValueSubject<Frame::Number>::create(Frame::invalid);
            p.volume = ValueSubject<float>::create(1.f);
            p.mute = ValueSubject<bool>::create(false);
            p.threadCount = ValueSubject<size_t>::create(4);
            p.cachedFrames = ListSubject<Frame::Range>::create();

            p.videoQueueMax = ValueSubject<size_t>::create();
            p.audioQueueMax = ValueSubject<size_t>::create();
            p.videoQueueCount = ValueSubject<size_t>::create();
            p.audioQueueCount = ValueSubject<size_t>::create();

            p.queueTimer = Time::Timer::create(context);
            p.queueTimer->setRepeating(true);
            p.playbackTimer = Time::Timer::create(context);
            p.playbackTimer->setRepeating(true);
            p.realSpeedTimer = Time::Timer::create(context);
            p.realSpeedTimer->setRepeating(true);
            p.cachedFramesTimer = Time::Timer::create(context);
            p.cachedFramesTimer->setRepeating(true);
            p.debugTimer = Time::Timer::create(context);
            p.debugTimer->setRepeating(true);

            try
            {
                p.rtAudio.reset(new RtAudio);
            }
            catch (const std::exception& e)
            {
                std::stringstream ss;
                ss << "djv::ViewApp::Media " << DJV_TEXT("RtAudio cannot be initialized") << ". " << e.what();
                auto logSystem = context->getSystemT<LogSystem>();
                logSystem->log("djv::ViewApp::Media", ss.str(), LogLevel::Error);
            }

            _open();

            auto weak = std::weak_ptr<Media>(std::dynamic_pointer_cast<Media>(shared_from_this()));
            p.queueTimer->start(
                Time::getMilliseconds(Time::TimerValue::VeryFast),
                [weak](float)
                {
                    if (auto media = weak.lock())
                    {
                        media->_queueUpdate();
                    }
                });
        }

        Media::Media() :
            _p(new Private)
        {}

        Media::~Media()
        {
            DJV_PRIVATE_PTR();
            p.rtAudio.reset();
        }

        std::shared_ptr<Media> Media::create(const Core::FileSystem::FileInfo& fileInfo, Context* context)
        {
            auto out = std::shared_ptr<Media>(new Media);
            out->_init(fileInfo, context);
            return out;
        }

        const Core::FileSystem::FileInfo& Media::getFileInfo() const
        {
            return _p->fileInfo;
        }

        std::shared_ptr<IValueSubject<AV::IO::Info> > Media::observeInfo() const
        {
            return _p->info;
        }

        std::shared_ptr<Core::IValueSubject<bool> > Media::observeReload() const
        {
            return _p->reload;
        }

        void Media::reload()
        {
            _open();
        }

        std::shared_ptr<IValueSubject<size_t> > Media::observeLayer() const
        {
            return _p->layer;
        }

        void Media::setLayer(size_t value)
        {
            if (_p->layer->setIfChanged(value))
            {
                _open();
            }
        }

        void Media::nextLayer()
        {
            size_t layer = _p->layer->get();
            ++layer;
            if (layer >= _p->info->get().video.size())
            {
                layer = 0;
            }
            setLayer(layer);
        }

        void Media::prevLayer()
        {
            size_t layer = _p->layer->get();
            const size_t size = _p->info->get().video.size();
            if (layer > 0)
            {
                --layer;
            }
            else if (size > 0)
            {
                layer = size - 1;
            }
            else
            {
                layer = 0;
            }
            setLayer(layer);
        }

        std::shared_ptr<IValueSubject<std::shared_ptr<AV::Image::Image> > > Media::observeCurrentImage() const
        {
            return _p->currentImage;
        }

        std::shared_ptr<IValueSubject<Time::Speed> > Media::observeSpeed() const
        {
            return _p->speed;
        }

        std::shared_ptr<IValueSubject<Time::Speed> > Media::observeDefaultSpeed() const
        {
            return _p->defaultSpeed;
        }

        std::shared_ptr<IValueSubject<float> > Media::observeRealSpeed() const
        {
            return _p->realSpeed;
        }

        std::shared_ptr<IValueSubject<bool> > Media::observePlayEveryFrame() const
        {
            return _p->playEveryFrame;
        }

        std::shared_ptr<IValueSubject<Frame::Sequence> > Media::observeSequence() const
        {
            return _p->sequence;
        }

        std::shared_ptr<IValueSubject<Frame::Number> > Media::observeCurrentFrame() const
        {
            return _p->currentFrame;
        }

        std::shared_ptr<IValueSubject<Playback> > Media::observePlayback() const
        {
            return _p->playback;
        }

        std::shared_ptr<IValueSubject<PlaybackMode> > Media::observePlaybackMode() const
        {
            return _p->playbackMode;
        }

        std::shared_ptr<IValueSubject<bool> > Media::observeInOutPointsEnabled() const
        {
            return _p->inOutPointsEnabled;
        }

        std::shared_ptr<IValueSubject<Frame::Number> > Media::observeInPoint() const
        {
            return _p->inPoint;
        }

        std::shared_ptr<IValueSubject<Frame::Number> > Media::observeOutPoint() const
        {
            return _p->outPoint;
        }

        void Media::setSpeed(const Time::Speed& value)
        {
            DJV_PRIVATE_PTR();
            if (p.speed->setIfChanged(value))
            {
                p.frameOffset = p.currentFrame->get();
                p.realSpeedFrameCount = 0;
                p.startTime = std::chrono::system_clock::now();
                p.realSpeedTime = p.startTime;
            }
        }

        void Media::setPlayEveryFrame(bool value)
        {
            _p->playEveryFrame->setIfChanged(value);
        }

        void Media::setCurrentFrame(Frame::Number value)
        {
            DJV_PRIVATE_PTR();
            Frame::Number start = 0;
            const size_t size = p.sequence->get().getSize();
            Frame::Number end = static_cast<Frame::Number>(size) - 1;
            if (p.inOutPointsEnabled->get())
            {
                start = p.inPoint->get();
                end = p.outPoint->get();
            }
            Frame::Number tmp = value;
            if (tmp > end)
            {
                tmp = 0;
            }
            if (tmp < 0)
            {
                tmp = end;
            }
            if (p.currentFrame->setIfChanged(tmp))
            {
                setPlayback(Playback::Stop);
                _seek(p.currentFrame->get());
            }
        }

        void Media::inPoint()
        {
            start();
        }

        void Media::outPoint()
        {
            end();
        }

        void Media::start()
        {
            setCurrentFrame(0);
        }

        void Media::end()
        {
            DJV_PRIVATE_PTR();
            const size_t size = p.sequence->get().getSize();
            const Frame::Number frame = size > 0 ? (size - 1) : 0;
            setCurrentFrame(frame);
        }

        void Media::nextFrame(size_t value)
        {
            DJV_PRIVATE_PTR();
            const Frame::Number frame = p.currentFrame->get();
            setCurrentFrame(frame + static_cast<Frame::Number>(value));
        }

        void Media::prevFrame(size_t value)
        {
            DJV_PRIVATE_PTR();
            const Frame::Number frame = p.currentFrame->get();
            setCurrentFrame(frame - static_cast<Frame::Number>(value));
        }

        void Media::setPlayback(Playback value)
        {
            if (_p->playback->setIfChanged(value))
            {
                _playbackUpdate();
            }
        }

        void Media::setPlaybackMode(PlaybackMode value)
        {
            _p->playbackMode->setIfChanged(value);
        }

        void Media::setInOutPointsEnabled(bool value)
        {
            _p->inOutPointsEnabled->setIfChanged(value);
        }

        void Media::setInPoint(Frame::Number value)
        {
            _p->inPoint->setIfChanged(value);
        }

        void Media::setOutPoint(Frame::Number value)
        {
            _p->outPoint->setIfChanged(value);
        }

        void Media::resetInPoint()
        {
            _p->inPoint->setIfChanged(0);
        }

        void Media::resetOutPoint()
        {
            _p->outPoint->setIfChanged(0);
        }

        std::shared_ptr<IValueSubject<float> > Media::observeVolume() const
        {
            return _p->volume;
        }

        std::shared_ptr<IValueSubject<bool> > Media::observeMute() const
        {
            return _p->mute;
        }

        void Media::setVolume(float value)
        {
            if (_p->volume->setIfChanged(Math::clamp(value, 0.f, 1.f)))
            {
                _volumeUpdate();
            }
        }

        void Media::setMute(bool value)
        {
            if (_p->mute->setIfChanged(value))
            {
                _volumeUpdate();
            }
        }

        std::shared_ptr<IValueSubject<size_t> > Media::observeThreadCount() const
        {
            return _p->threadCount;
        }

        void Media::setThreadCount(size_t value)
        {
            DJV_PRIVATE_PTR();
            if (p.threadCount->setIfChanged(value))
            {
                if (p.read)
                {
                    p.read->setThreadCount(value);
                }
            }
        }

        bool Media::hasCache() const
        {
            DJV_PRIVATE_PTR();
            return p.read ? p.read->hasCache() : false;
        }

        std::shared_ptr<Core::IListSubject<Frame::Range> > Media::observeCachedFrames() const
        {
            return _p->cachedFrames;
        }

        void Media::setCacheEnabled(bool value)
        {
            DJV_PRIVATE_PTR();
            if (p.read)
            {
                p.read->setCacheEnabled(value);
            }
        }

        void Media::setCacheMax(size_t value)
        {
            DJV_PRIVATE_PTR();
            if (p.read)
            {
                p.read->setCacheMax(value);
            }
        }

        std::shared_ptr<IValueSubject<size_t> > Media::observeVideoQueueMax() const
        {
            return _p->videoQueueMax;
        }

        std::shared_ptr<IValueSubject<size_t> > Media::observeAudioQueueMax() const
        {
            return _p->audioQueueMax;
        }

        std::shared_ptr<IValueSubject<size_t> > Media::observeVideoQueueCount() const
        {
            return _p->videoQueueCount;
        }

        std::shared_ptr<IValueSubject<size_t> > Media::observeAudioQueueCount() const
        {
            return _p->audioQueueCount;
        }

        bool Media::_hasAudio() const
        {
            DJV_PRIVATE_PTR();
            return p.audioInfo.info.isValid() && p.rtAudio;
        }

        void Media::_open()
        {
            DJV_PRIVATE_PTR();
            try
            {
                AV::IO::ReadOptions options;
                options.layer = p.layer->get();
                options.videoQueueSize = videoQueueSize;
                auto io = p.context->getSystemT<AV::IO::System>();
                p.read = io->read(p.fileInfo, options);
                p.read->setThreadCount(p.threadCount->get());
                
                const auto info = p.read->getInfo().get();
                p.info->setIfChanged(info);
                Time::Speed speed;
                Frame::Sequence sequence;
                const auto& video = info.video;
                if (video.size())
                {
                    p.videoInfo = video[0];
                    speed = video[0].speed;
                    sequence = video[0].sequence;
                }
                const auto& audio = info.audio;
                if (audio.size())
                {
                    p.audioInfo = audio[0];
                }
                {
                    std::stringstream ss;
                    ss << p.fileInfo << " sequence: " << sequence.getSize();
                    auto logSystem = p.context->getSystemT<LogSystem>();
                    logSystem->log("djv::ViewApp::Media", ss.str());
                }
                p.info->setIfChanged(info);
                p.speed->setIfChanged(speed);
                p.defaultSpeed->setIfChanged(speed);
                p.sequence->setIfChanged(sequence);

                if (p.rtAudio)
                {
                    p.rtAudio->closeStream();
                    RtAudio::StreamParameters rtParameters;
                    rtParameters.deviceId = 0;
                    rtParameters.nChannels = p.audioInfo.info.channelCount;
                    RtAudioFormat rtFormat = 0;
                    switch (p.audioInfo.info.type)
                    {
                    case AV::Audio::Type::S16: rtFormat = RTAUDIO_SINT16; break;
                    case AV::Audio::Type::S32: rtFormat = RTAUDIO_SINT32; break;
                    case AV::Audio::Type::F32: rtFormat = RTAUDIO_FLOAT32; break;
                    }
                    unsigned int rtBufferFrames = bufferFrameCount;
                    try
                    {
                        p.rtAudio->openStream(
                            &rtParameters,
                            nullptr,
                            rtFormat,
                            p.audioInfo.info.sampleRate,
                            &rtBufferFrames,
                            _rtAudioCallback,
                            this,
                            nullptr,
                            _rtAudioErrorCallback);
                    }
                    catch (const std::exception& e)
                    {
                        std::stringstream ss;
                        ss << "djv::ViewApp::Media " << DJV_TEXT("cannot open audio stream") << ". " << e.what();
                        auto logSystem = p.context->getSystemT<LogSystem>();
                        logSystem->log("djv::ViewApp::Media", ss.str(), LogLevel::Error);
                    }
                }

                auto weak = std::weak_ptr<Media>(std::dynamic_pointer_cast<Media>(shared_from_this()));
                p.cachedFramesTimer->start(
                    Time::getMilliseconds(Time::TimerValue::Medium),
                    [weak](float)
                    {
                        if (auto media = weak.lock())
                        {
                            if (media->_p->read)
                            {
                                const auto& frames = media->_p->read->getCachedFrames();
                                media->_p->cachedFrames->setIfChanged(frames);
                            }
                        }
                    });

                p.debugTimer->start(
                    Time::getMilliseconds(Time::TimerValue::Medium),
                    [weak](float)
                    {
                        if (auto media = weak.lock())
                        {
                            if (media->_p->read)
                            {
                                size_t videoQueueMax   = 0;
                                size_t videoQueueCount = 0;
                                size_t audioQueueMax   = 0;
                                size_t audioQueueCount = 0;
                                {
                                    std::lock_guard<std::mutex> lock(media->_p->read->getMutex());
                                    const auto& videoQueue = media->_p->read->getVideoQueue();
                                    const auto& audioQueue = media->_p->read->getAudioQueue();
                                    videoQueueMax   = videoQueue.getMax();
                                    videoQueueCount = videoQueue.getCount();
                                    audioQueueMax   = audioQueue.getMax();
                                    audioQueueCount = audioQueue.getCount();
                                }
                                media->_p->videoQueueMax->setAlways(videoQueueMax);
                                media->_p->videoQueueCount->setAlways(videoQueueCount);
                                media->_p->audioQueueMax->setAlways(audioQueueMax);
                                media->_p->audioQueueCount->setAlways(audioQueueCount);
                            }
                        }
                    });
            }
            catch (const std::exception& e)
            {
                std::stringstream ss;
                ss << "djv::ViewApp::Media " << DJV_TEXT("cannot open") << " '" << p.fileInfo << "'. " << e.what();
                auto logSystem = p.context->getSystemT<LogSystem>();
                logSystem->log("djv::ViewApp::Media", ss.str(), LogLevel::Error);
            }
            
            _seek(p.currentFrame->get());

            p.reload->setAlways(true);
        }

        void Media::_seek(Frame::Number value)
        {
            DJV_PRIVATE_PTR();
            if (p.read)
            {
                p.read->seek(value, p.ioDirection);
            }
            p.audioData.reset();
            p.audioDataOffset = 0;
            p.audioDataTotal = 0;
            p.audioDataTimeOffset = std::chrono::system_clock::now();
            p.frameOffset = p.currentFrame->get();
            p.startTime = std::chrono::system_clock::now();
            p.realSpeedTime = p.startTime;
            p.realSpeedFrameCount = 0;
            if (p.rtAudio)
            {
                try
                {
                    p.rtAudio->abortStream();
                    p.rtAudio->setStreamTime(0.0);
                }
                catch (const std::exception& e)
                {
                    std::stringstream ss;
                    ss << "djv::ViewApp::Media " << DJV_TEXT("cannot stop audio stream") << ". " << e.what();
                    auto logSystem = p.context->getSystemT<LogSystem>();
                    logSystem->log("djv::ViewApp::Media", ss.str(), LogLevel::Error);
                }
            }
        }

        void Media::_playbackUpdate()
        {
            DJV_PRIVATE_PTR();
            bool forward = false;
            switch (p.playback->get())
            {
            case Playback::Stop:
                if (p.rtAudio)
                {
                    try
                    {
                        p.rtAudio->abortStream();
                        p.rtAudio->setStreamTime(0.0);
                    }
                    catch (const std::exception& e)
                    {
                        std::stringstream ss;
                        ss << "djv::ViewApp::Media " << DJV_TEXT("cannot stop audio stream") << ". " << e.what();
                        auto logSystem = p.context->getSystemT<LogSystem>();
                        logSystem->log("djv::ViewApp::Media", ss.str(), LogLevel::Error);
                    }
                }
                p.playbackTimer->stop();
                p.realSpeedTimer->stop();
                _seek(p.currentFrame->get());
                break;
            case Playback::Forward: forward = true;
            case Playback::Reverse:
            {
                p.ioDirection = forward ? AV::IO::Direction::Forward : AV::IO::Direction::Reverse;
                _seek(p.currentFrame->get());
                p.audioData.reset();
                p.audioDataOffset = 0;
                p.audioDataTotal = 0;
                p.audioDataTimeOffset = std::chrono::system_clock::now();
                p.frameOffset = p.currentFrame->get();
                p.startTime = std::chrono::system_clock::now();
                p.realSpeedTime = p.startTime;
                p.realSpeedFrameCount = 0;
                if (p.rtAudio)
                {
                    try
                    {
                        p.rtAudio->startStream();
                    }
                    catch (const std::exception& e)
                    {
                        std::stringstream ss;
                        ss << "djv::ViewApp::Media " << DJV_TEXT("cannot start audio stream") << ". " << e.what();
                        auto logSystem = p.context->getSystemT<LogSystem>();
                        logSystem->log("djv::ViewApp::Media", ss.str(), LogLevel::Error);
                    }
                }
                auto weak = std::weak_ptr<Media>(std::dynamic_pointer_cast<Media>(shared_from_this()));
                p.playbackTimer->start(
                    Time::getMilliseconds(Time::TimerValue::VeryFast),
                    [weak](float)
                {
                    if (auto media = weak.lock())
                    {
                        media->_playbackTick();
                    }
                });
                p.realSpeedTimer->start(
                    Time::getMilliseconds(Time::TimerValue::Slow),
                    [weak](float)
                    {
                        if (auto media = weak.lock())
                        {
                            const auto now = std::chrono::system_clock::now();
                            std::chrono::duration<double> delta = now - media->_p->realSpeedTime;
                            media->_p->realSpeed->setIfChanged(delta.count() ? (media->_p->realSpeedFrameCount / static_cast<float>(delta.count())) : 0.f);
                            media->_p->realSpeedTime = now;
                            media->_p->realSpeedFrameCount = 0;
                        }
                    });
                break;
            }
            default: break;
            }
        }

        void Media::_playbackTick()
        {
            DJV_PRIVATE_PTR();
            const Playback playback = p.playback->get();
            bool forward = false;
            switch (playback)
            {
            case Playback::Forward: forward = true;
            case Playback::Reverse:
            {
                Frame::Number frame = Frame::invalid;
                const auto& speed = p.speed->get();
                const auto now = std::chrono::system_clock::now();
                if (forward && _hasAudio())
                {
                    std::chrono::duration<double> delta = now - _p->audioDataTimeOffset;
                    frame = p.frameOffset +
                        Time::scale(
                            p.audioDataTotal,
                            Math::Rational(1, static_cast<int>(p.audioInfo.info.sampleRate)),
                            speed.swap()) +
                        delta.count() * speed.toFloat();
                }
                else
                {
                    std::chrono::duration<double> delta = now - p.startTime;
                    Frame::Number elapsed = static_cast<Frame::Number>(delta.count() * speed.toFloat());
                    switch (playback)
                    {
                    case Playback::Forward: frame = p.frameOffset + elapsed; break;
                    case Playback::Reverse: frame = p.frameOffset - elapsed; break;
                    default: break;
                    }
                }

                Frame::Number start = 0;
                const Frame::Sequence& sequence = p.sequence->get();
                Frame::Number end = static_cast<Frame::Number>(sequence.getSize()) - 1;
                if (p.inOutPointsEnabled->get())
                {
                    start = p.inPoint->get();
                    end = p.outPoint->get();
                }
                if ((Playback::Forward == playback && frame >= end) ||
                    (Playback::Reverse == playback && frame <= start))
                {
                    switch (p.playbackMode->get())
                    {
                    case PlaybackMode::Once:
                        switch (p.playback->get())
                        {
                        case Playback::Forward: frame = end;   break;
                        case Playback::Reverse: frame = start; break;
                        default: break;
                        }
                        setPlayback(Playback::Stop);
                        setCurrentFrame(frame);
                        break;
                    case PlaybackMode::Loop:
                    {
                        Playback playback = p.playback->get();
                        switch (playback)
                        {
                        case Playback::Forward: frame = start; break;
                        case Playback::Reverse: frame = end;   break;
                        default: break;
                        }
                        setPlayback(Playback::Stop);
                        setCurrentFrame(frame);
                        setPlayback(playback);
                        break;
                    }
                    case PlaybackMode::PingPong:
                    {
                        Playback playback = p.playback->get();
                        switch (playback)
                        {
                        case Playback::Forward: frame = end;   break;
                        case Playback::Reverse: frame = start; break;
                        default: break;
                        }
                        setPlayback(Playback::Stop);
                        setCurrentFrame(frame);
                        setPlayback(Playback::Forward == playback ? Playback::Reverse : Playback::Forward);
                        break;
                    }
                    default: break;
                    }
                }
                p.currentFrame->setIfChanged(frame);
                break;
            }
            default: break;
            }
        }

        void Media::_queueUpdate()
        {
            DJV_PRIVATE_PTR();
            if (p.read)
            {
                // Update the video queue.
                const bool forward = Playback::Forward == p.playback->get();
                const Frame::Number currentFrame = p.currentFrame->get();
                std::shared_ptr<AV::Image::Image> image;
                {
                    std::lock_guard<std::mutex> lock(p.read->getMutex());
                    auto& queue = p.read->getVideoQueue();
                    while (!queue.isEmpty() &&
                        (forward ? (queue.getFrame().frame < currentFrame) : (queue.getFrame().frame > currentFrame)))
                    {
                        auto frame = queue.popFrame();
                        p.realSpeedFrameCount = p.realSpeedFrameCount + 1;
                    }
                    if (!queue.isEmpty())
                    {
                        image = queue.getFrame().image;
                    }
                }
                if (image)
                {
                    p.currentImage->setIfChanged(image);
                }
            }
        }

        void Media::_volumeUpdate()
        {
            DJV_PRIVATE_PTR();
            if (_hasAudio())
            {
                const float volume = !p.mute->get() ? p.volume->get() : 0.f;
            }
        }
        
        int Media::_rtAudioCallback(
            void* outputBuffer,
            void* inputBuffer,
            unsigned int nFrames,
            double streamTime,
            RtAudioStreamStatus status,
            void* userData)
        {
            Media* media = reinterpret_cast<Media*>(userData);
            const auto& info = media->_p->audioInfo;

            size_t outputSampleCount = static_cast<size_t>(nFrames);
            size_t sampleCount = 0;
            const size_t sampleByteCount = info.info.channelCount * AV::Audio::getByteCount(info.info.type);

            if (media->_p->audioData)
            {
                sampleCount += media->_p->audioData->getSampleCount() - media->_p->audioDataOffset;
            }

            std::vector<AV::IO::AudioFrame> frames;
            {
                std::lock_guard<std::mutex> lock(media->_p->read->getMutex());
                auto& queue = media->_p->read->getAudioQueue();
                while (!queue.isEmpty() && sampleCount < outputSampleCount)
                {
                    auto frame = queue.getFrame();
                    frames.push_back(frame);
                    queue.popFrame();
                    sampleCount += frame.audio->getSampleCount();
                }
            }

            uint8_t* p = reinterpret_cast<uint8_t*>(outputBuffer);
            if (media->_p->audioData)
            {
                const size_t size = std::min(media->_p->audioData->getSampleCount() - media->_p->audioDataOffset, outputSampleCount);
                memcpy(
                    p,
                    media->_p->audioData->getData() + media->_p->audioDataOffset * sampleByteCount,
                    size * sampleByteCount);
                p += size * sampleByteCount;
                media->_p->audioDataOffset += size;
                media->_p->audioDataTotal += size;
                media->_p->audioDataTimeOffset = std::chrono::system_clock::now();
                outputSampleCount -= size;
                if (media->_p->audioDataOffset >= media->_p->audioData->getSampleCount())
                {
                    media->_p->audioData.reset();
                    media->_p->audioDataOffset = 0;
                }
            }

            for (const auto& i : frames)
            {
                media->_p->audioData = i.audio;
                size_t size = std::min(i.audio->getSampleCount(), outputSampleCount);
                memcpy(
                    p,
                    i.audio->getData(),
                    size * sampleByteCount);
                p += size * sampleByteCount;
                media->_p->audioDataOffset = size;
                media->_p->audioDataTotal += size;
                media->_p->audioDataTimeOffset = std::chrono::system_clock::now();
                outputSampleCount -= size;
            }

            const size_t zero = (nFrames * sampleByteCount) - (p - reinterpret_cast<uint8_t*>(outputBuffer));
            if (zero)
            {
                //! \todo What is the correct value for silence?
                memset(p, 0, zero);
            }

            return 0;
        }

        void Media::_rtAudioErrorCallback(
            RtAudioError::Type type,
            const std::string& errorText)
        {}

    } // namespace ViewApp
} // namespace djv


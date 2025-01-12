// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvSystem/DirectoryWatcher.h>

#include <djvSystem/Context.h>
#include <djvSystem/LogSystem.h>
#include <djvSystem/Path.h>
#include <djvSystem/Timer.h>

#include <djvCore/Error.h>
#include <djvCore/String.h>
#include <djvCore/StringFormat.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

using namespace djv::Core;

namespace djv
{
    namespace System
    {
        namespace File
        {
            struct DirectoryWatcher::Private
            {
                Path path;
                bool changed = false;
                std::condition_variable changedCV;
                std::mutex mutex;
                std::thread thread;
                std::atomic<bool> running = true;
                std::function<void(void)> callback;
                std::shared_ptr<Timer> timer;
            };

            void DirectoryWatcher::_init(const std::shared_ptr<Context>& context)
            {
                DJV_PRIVATE_PTR();
                auto contextWeak = std::weak_ptr<Context>(context);
                _p->thread = std::thread(
                    [this, contextWeak]
                {
                    DJV_PRIVATE_PTR();
                    Path path;
                    HANDLE changeHandle = INVALID_HANDLE_VALUE;
                    const Time::Duration timeout = getTimerDuration(TimerValue::Medium);
                    while (p.running)
                    {
                        bool pathChanged = false;
                        {
                            std::lock_guard<std::mutex> lock(p.mutex);
                            if (p.path != path)
                            {
                                path = p.path;
                                pathChanged = true;
                            }
                        }
                        if (pathChanged)
                        {
                            if (changeHandle && changeHandle != INVALID_HANDLE_VALUE)
                            {
                                FindCloseChangeNotification(changeHandle);
                                changeHandle = INVALID_HANDLE_VALUE;
                            }
                            //! \todo What about servers?
                            if (!path.isServer())
                            {
                                try
                                {
                                    changeHandle = FindFirstChangeNotificationW(
                                        String::toWide(path.get()).c_str(),
                                        FALSE,
                                        FILE_NOTIFY_CHANGE_FILE_NAME |
                                        FILE_NOTIFY_CHANGE_DIR_NAME |
                                        FILE_NOTIFY_CHANGE_SIZE |
                                        FILE_NOTIFY_CHANGE_LAST_WRITE);
                                    if (INVALID_HANDLE_VALUE == changeHandle)
                                    {
                                        if (auto context = contextWeak.lock())
                                        {
                                            auto logSystem = context->getSystemT<LogSystem>();
                                            logSystem->log(
                                                "djv::System::File::DirectoryWatcher",
                                                String::Format("{0}: {1}").arg(path.get()).arg(Error::getLastError()),
                                                LogLevel::Error);
                                        }
                                    }
                                }
                                catch (const std::exception& e)
                                {
                                    if (auto context = contextWeak.lock())
                                    {
                                        auto logSystem = context->getSystemT<LogSystem>();
                                        logSystem->log(
                                            "djv::System::File::DirectoryWatcher",
                                            String::Format("{0}: {1}").arg(path.get()).arg(e.what()),
                                            LogLevel::Error);
                                    }
                                }
                            }
                        }

                        if (changeHandle && changeHandle != INVALID_HANDLE_VALUE)
                        {
                            const DWORD status = WaitForSingleObject(changeHandle, 0);
                            switch (status)
                            {
                            case WAIT_OBJECT_0:
                                FindNextChangeNotification(changeHandle);
                                {
                                    std::lock_guard<std::mutex> lock(p.mutex);
                                    p.changed = true;
                                }
                                break;
                            }
                        }

                        std::this_thread::sleep_for(timeout);
                    }

                    if (changeHandle && changeHandle != INVALID_HANDLE_VALUE)
                    {
                        FindCloseChangeNotification(changeHandle);
                        changeHandle = INVALID_HANDLE_VALUE;
                    }
                });

                p.timer = Timer::create(context);
                p.timer->setRepeating(true);
                p.timer->start(
                    getTimerDuration(TimerValue::Medium),
                    [this](const std::chrono::steady_clock::time_point&, const Time::Duration&)
                {
                    DJV_PRIVATE_PTR();
                    bool changed = false;
                    {
                        std::unique_lock<std::mutex> lock(p.mutex);
                        if (p.changedCV.wait_for(
                            lock,
                            std::chrono::milliseconds(0),
                            [this]
                        {
                            return _p->changed;
                        }))
                        {
                            changed = true;
                            p.changed = false;
                        }
                    }
                    if (changed && p.callback)
                    {
                        p.callback();
                    }
                });
            }

            DirectoryWatcher::DirectoryWatcher() :
                _p(new Private)
            {}

            DirectoryWatcher::~DirectoryWatcher()
            {
                DJV_PRIVATE_PTR();
                p.running = false;
                if (p.thread.joinable())
                {
                    p.thread.join();
                }
            }

            std::shared_ptr<DirectoryWatcher> DirectoryWatcher::create(const std::shared_ptr<Context>& context)
            {
                auto out = std::shared_ptr<DirectoryWatcher>(new DirectoryWatcher);
                out->_init(context);
                return out;
            }

            const Path& DirectoryWatcher::getPath() const
            {
                return _p->path;
            }

            void DirectoryWatcher::setPath(const Path& value)
            {
                DJV_PRIVATE_PTR();
                if (value == p.path)
                    return;
                {
                    std::lock_guard<std::mutex> lock(p.mutex);
                    p.path = value;
                    p.changed = true;
                }
                if (p.callback)
                {
                    p.callback();
                }
            }

            void DirectoryWatcher::setCallback(const std::function<void(void)>& value)
            {
                _p->callback = value;
            }

        } // namespace File
    } // namespace System
} // namespace djv

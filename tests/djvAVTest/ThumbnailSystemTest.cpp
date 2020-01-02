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

#include <djvAVTest/ThumbnailSystemTest.h>

#include <djvAV/IO.h>
#include <djvAV/ThumbnailSystem.h>

#include <djvCore/Context.h>
#include <djvCore/ResourceSystem.h>
#include <djvCore/Timer.h>

using namespace djv::Core;
using namespace djv::AV;

namespace djv
{
    namespace AVTest
    {
        ThumbnailSystemTest::ThumbnailSystemTest(const std::shared_ptr<Core::Context>& context) :
            ITickTest("djv::AVTest::ThumbnailSystemTest", context)
        {}
        
        void ThumbnailSystemTest::run(const std::vector<std::string>& args)
        {
            if (auto context = getContext().lock())
            {
                auto resourceSystem = context->getSystemT<ResourceSystem>();
                const FileSystem::FileInfo fileInfo(FileSystem::Path(
                    resourceSystem->getPath(FileSystem::ResourcePath::Icons),
                    "96DPI/djvIconFile.png"));
                auto system = context->getSystemT<ThumbnailSystem>();
                auto infoFuture = system->getInfo(fileInfo);
                auto imageFuture = system->getImage(fileInfo, Image::Size(32, 32));
                
                auto infoCancelFuture = system->getInfo(fileInfo);
                auto imageCancelFuture = system->getImage(fileInfo, Image::Size(32, 32));
                system->cancelInfo(infoCancelFuture.uid);
                system->cancelImage(imageCancelFuture.uid);
                
                IO::Info info;
                std::shared_ptr<Image::Image> image;
                while (
                    infoFuture.future.valid() ||
                    imageFuture.future.valid())
                {
                    _tickFor(Time::getTime(Time::TimerValue::Fast));
                    if (infoFuture.future.valid() &&
                        infoFuture.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    {
                        info = infoFuture.future.get();
                    }
                    if (imageFuture.future.valid() &&
                        imageFuture.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                    {
                        image = imageFuture.future.get();
                    }
                }
                
                if (info.video.size())
                {
                    std::stringstream ss;
                    ss << "info: " << info.video[0].info.size;
                    _print(ss.str());
                }
                if (image)
                {
                    std::stringstream ss;
                    ss << "image: " << image->getSize();
                    _print(ss.str());
                }
                
                {
                    std::stringstream ss;
                    ss << "info cache percentage: " << system->getInfoCachePercentage();
                    _print(ss.str());
                }
                {
                    std::stringstream ss;
                    ss << "image cache percentage: " << system->getImageCachePercentage();
                    _print(ss.str());
                }
                
                system->clearCache();
            }
        }
        
    } // namespace AVTest
} // namespace djv


// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#pragma once

#include <djvCore/ListObserver.h>
#include <djvCore/ValueObserver.h>

namespace djv
{
    namespace System
    {
        namespace File
        {
            class Info;

            //! This class provides a model for keeping a list of recently used files.
            class RecentFilesModel : public std::enable_shared_from_this<RecentFilesModel>
            {
                void _init();
                RecentFilesModel();

            public:
                static std::shared_ptr<RecentFilesModel> create();

                std::shared_ptr<Core::Observer::IListSubject<Info> > observeFiles() const;
                
                void setFiles(std::vector<Info>);
                void addFile(const Info&);

                std::shared_ptr<Core::Observer::IValueSubject<size_t> > observeFilesMax() const;
                
                void setFilesMax(size_t);

            private:
                DJV_PRIVATE();
            };

        } // namespace File
    } // namespace System
} // namespace djv


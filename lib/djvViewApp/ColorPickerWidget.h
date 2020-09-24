// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#pragma once

#include <djvViewApp/ColorPickerData.h>
#include <djvViewApp/MDIWidget.h>

#include <djvImage/Pixel.h>

namespace djv
{
    namespace ViewApp
    {
        //! This class provides the color picker widget.
        class ColorPickerWidget : public MDIWidget
        {
            DJV_NON_COPYABLE(ColorPickerWidget);

        protected:
            void _init(const std::shared_ptr<System::Context>&);
            ColorPickerWidget();

        public:
            ~ColorPickerWidget() override;

            static std::shared_ptr<ColorPickerWidget> create(const std::shared_ptr<System::Context>&);
            
            void setCurrentTool(bool);

            const glm::vec2& getPickerPos() const;

            void setPickerPos(const glm::vec2&);

        protected:
            void _initEvent(System::Event::Init &) override;

        private:
            void _sampleUpdate();
            void _widgetUpdate();

            DJV_PRIVATE();
        };

    } // namespace ViewApp
} // namespace djv


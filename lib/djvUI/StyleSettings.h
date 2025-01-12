// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#pragma once

#include <djvUI/ISettings.h>
#include <djvUI/Style.h>

#include <djvCore/MapObserver.h>
#include <djvCore/ValueObserver.h>

namespace djv
{
    namespace UI
    {
        namespace Settings
        {
            //! Style settings.
            class Style : public ISettings
            {
                DJV_NON_COPYABLE(Style);

            protected:
                void _init(const std::shared_ptr<System::Context>&);

                Style();

            public:
                ~Style() override;

                static std::shared_ptr<Style> create(const std::shared_ptr<System::Context>&);

                //! \name Color Palette
                ///@{

                std::shared_ptr<Core::Observer::IMapSubject<std::string, UI::Style::Palette> > observePalettes() const;
                std::shared_ptr<Core::Observer::IValueSubject<UI::Style::Palette> > observeCurrentPalette() const;
                std::shared_ptr<Core::Observer::IValueSubject<std::string> > observeCurrentPaletteName() const;
                std::shared_ptr<Core::Observer::IValueSubject<float> > observeBrightness() const;
                std::shared_ptr<Core::Observer::IValueSubject<float> > observeContrast() const;

                void setCurrentPalette(const std::string&);
                void setBrightness(float);
                void setContrast(float);

                ///@}

                //! \name Size Metrics
                ///@{

                std::shared_ptr<Core::Observer::IMapSubject<std::string, UI::Style::Metrics> > observeMetrics() const;
                std::shared_ptr<Core::Observer::IValueSubject<UI::Style::Metrics> > observeCurrentMetrics() const;
                std::shared_ptr<Core::Observer::IValueSubject<std::string> > observeCurrentMetricsName() const;
                
                void setCurrentMetrics(const std::string&);

                ///@}

                //! \name Fonts
                ///@{

                std::shared_ptr<Core::Observer::IValueSubject<std::string> > observeCurrentFont() const;

                ///@}

                void load(const rapidjson::Value&) override;
                rapidjson::Value save(rapidjson::Document::AllocatorType&) override;

            private:
                void _currentFontUpdate();

                DJV_PRIVATE();
            };

        } // namespace Settings
    } // namespace UI
} // namespace djv

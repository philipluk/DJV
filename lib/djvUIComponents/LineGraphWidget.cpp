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

#include <djvUIComponents/LineGraphWidget.h>

#include <djvUI/Label.h>

#include <djvAV/Render2D.h>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        struct LineGraphWidget::Private
        {
            std::list<float> samples;
            size_t samplesSize = 0;
            FloatRange samplesRange = FloatRange(0.f, 0.f);
            size_t precision = 2;
            std::shared_ptr<Label> label;
        };

        void LineGraphWidget::_init(Context * context)
        {
            Widget::_init(context);

            DJV_PRIVATE_PTR();
            setClassName("djv::UI::LineGraphWidget");

            setBackgroundRole(ColorRole::Trough);

            p.label = Label::create(context);
            p.label->setFontSizeRole(MetricsRole::FontSmall);
            p.label->setBackgroundRole(ColorRole::Overlay);
            p.label->setMargin(MetricsRole::Border);
            addChild(p.label);
        }

        LineGraphWidget::LineGraphWidget() :
            _p(new Private)
        {}

        LineGraphWidget::~LineGraphWidget()
        {}

        std::shared_ptr<LineGraphWidget> LineGraphWidget::create(Context * context)
        {
            auto out = std::shared_ptr<LineGraphWidget>(new LineGraphWidget);
            out->_init(context);
            return out;
        }

        const FloatRange& LineGraphWidget::getSampleRange() const
        {
            return _p->samplesRange;
        }

        void LineGraphWidget::addSample(float value)
        {
            DJV_PRIVATE_PTR();
            p.samples.push_front(value);
            p.samplesRange.min = std::min(value, p.samplesRange.min);
            p.samplesRange.max = std::max(value, p.samplesRange.max);
            while (p.samples.size() > p.samplesSize)
            {
                p.samples.pop_back();
            }
            _updateWidget();
            _redraw();
        }

        void LineGraphWidget::resetSamples()
        {
            DJV_PRIVATE_PTR();
            p.samples.clear();
            p.samplesRange.zero();
            _updateWidget();
            _redraw();
        }

        void LineGraphWidget::setPrecision(size_t value)
        {
            DJV_PRIVATE_PTR();
            if (value == p.precision)
                return;
            p.precision = value;
            _updateWidget();
            _redraw();
        }

        void LineGraphWidget::_preLayoutEvent(Event::PreLayout & event)
        {
            auto style = _getStyle();
            const float tc = style->getMetric(MetricsRole::TextColumn);
            _setMinimumSize(glm::vec2(tc, tc / 3.f));
        }

        void LineGraphWidget::_layoutEvent(Event::Layout&)
        {
            DJV_PRIVATE_PTR();
            auto style = _getStyle();
            const BBox2f& g = getGeometry();
            const float b = style->getMetric(MetricsRole::Border) * 2.f;
            const float w = g.w();
            p.samplesSize = static_cast<size_t>(w > 0.f ? (g.w() / b) : 0.f);
            while (p.samples.size() > p.samplesSize)
            {
                p.samples.pop_back();
            }
            const glm::vec2 labelSize = p.label->getMinimumSize();
            p.label->setGeometry(BBox2f(g.max.x - labelSize.x, g.max.y - labelSize.y, labelSize.x, labelSize.y));
        }

        void LineGraphWidget::_paintEvent(Event::Paint& event)
        {
            Widget::_paintEvent(event);
            DJV_PRIVATE_PTR();
            auto style = _getStyle();
            const float b = style->getMetric(MetricsRole::Border) * 2.f;
            const BBox2f& g = getMargin().bbox(getGeometry(), style).margin(0, 0, 0, -b);
            auto render = _getRender();
            auto color1 = style->getColor(ColorRole::Checked);
            auto color2 = style->getColor(ColorRole::Checked);
            color2.setF32(color2.getF32(3) * .5f, 3);
            float x = g.min.x;
            const float range = p.samplesRange.max - p.samplesRange.min;
            for (const auto& i : p.samples)
            {
                float h = (i - p.samplesRange.min) / range * g.h();
                render->setFillColor(color1);
                render->drawRect(BBox2f(x, g.min.y + g.h() - h, b, b));
                render->setFillColor(color2);
                render->drawRect(BBox2f(x, g.min.y + g.h() - h + b, b, h));
                x += b;
            }
        }

        void LineGraphWidget::_localeEvent(Event::Locale& event)
        {
            _updateWidget();
        }

        void LineGraphWidget::_updateWidget()
        {
            DJV_PRIVATE_PTR();
            std::stringstream ss;
            ss.precision(p.precision);
            ss << std::fixed;
            float v = p.samples.size() ? p.samples.front() : 0.f;
            ss << _getText(DJV_TEXT("Min")) << ": " << p.samplesRange.min << ", " <<
                _getText(DJV_TEXT("Max")) << ": " << p.samplesRange.max << ", " <<
                _getText(DJV_TEXT("Value")) << ": " << v;
            p.label->setText(ss.str());
        }

    } // namespace UI
} // namespace djv


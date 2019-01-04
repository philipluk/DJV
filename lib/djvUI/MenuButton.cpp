//------------------------------------------------------------------------------
// Copyright (c) 2018 Darby Johnston
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

#include <djvUI/MenuButton.h>

#include <djvUI/Border.h>
#include <djvUI/Icon.h>
#include <djvUI/Label.h>
#include <djvUI/RowLayout.h>
#include <djvUI/StackLayout.h>

#include <djvAV/Render2DSystem.h>

//#pragma optimize("", off)

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace Button
        {
            struct Menu::Private
            {
                bool checked = false;
                std::shared_ptr<Icon> icon;
                std::shared_ptr<Label> label;
                std::shared_ptr<Layout::Horizontal> layout;
                std::shared_ptr<Layout::Border> border;
                std::map<Event::PointerID, bool> pointerHover;
                std::function<void(bool)> checkedCallback;
            };

            void Menu::_init(Context * context)
            {
                Widget::_init(context);

                setClassName("djv::UI::Button::Menu");
                setPointerEnabled(true);

                _p->icon = Icon::create(context);
                _p->icon->setVAlign(VAlign::Center);
                _p->icon->hide();

                _p->label = Label::create(context);
                _p->label->hide();

                _p->layout = Layout::Horizontal::create(context);
                _p->layout->setMargin(Layout::Margin(Style::MetricsRole::Margin, Style::MetricsRole::Margin, Style::MetricsRole::MarginSmall, Style::MetricsRole::MarginSmall));
                _p->layout->addWidget(_p->icon);
                _p->layout->addWidget(_p->label, Layout::RowStretch::Expand);

                _p->border = Layout::Border::create(context);
                _p->border->setBorderSize(Style::MetricsRole::None);
                _p->border->addWidget(_p->layout);
                _p->border->setParent(shared_from_this());
            }

            Menu::Menu() :
                _p(new Private)
            {}

            Menu::~Menu()
            {}

            std::shared_ptr<Menu> Menu::create(Context * context)
            {
                auto out = std::shared_ptr<Menu>(new Menu);
                out->_init(context);
                return out;
            }

            std::shared_ptr<Menu> Menu::create(const std::string& text, Context * context)
            {
                auto out = std::shared_ptr<Menu>(new Menu);
                out->_init(context);
                out->setText(text);
                return out;
            }

            std::shared_ptr<Menu> Menu::create(const std::string& text, const FileSystem::Path& icon, Context * context)
            {
                auto out = std::shared_ptr<Menu>(new Menu);
                out->_init(context);
                out->setIcon(icon);
                out->setText(text);
                return out;
            }

            const FileSystem::Path& Menu::getIcon() const
            {
                return _p->icon->getIcon();
            }

            void Menu::setIcon(const FileSystem::Path& value)
            {
                _p->icon->setIcon(value);
                _p->icon->setVisible(!value.isEmpty());
            }

            const std::string& Menu::getText() const
            {
                return _p->label->getText();
            }

            void Menu::setText(const std::string& value)
            {
                _p->label->setText(value);
                _p->label->setVisible(!value.empty());
            }

            bool Menu::isChecked() const
            {
                return _p->checked;
            }

            void Menu::setChecked(bool value)
            {
                if (value == _p->checked)
                    return;
                _p->checked = value;
                _redraw();
                if (_p->checkedCallback)
                {
                    _p->checkedCallback(_p->checked);
                }
            }

            void Menu::setCheckedCallback(const std::function<void(bool)> & callback)
            {
                _p->checkedCallback = callback;
            }

            bool Menu::hasBorder() const
            {
                return _p->border->getBorderSize() != Style::MetricsRole::Border;
            }

            void Menu::setBorder(bool value)
            {
                _p->border->setBorderSize(value ? Style::MetricsRole::Border : Style::MetricsRole::None);
            }

            void Menu::_preLayoutEvent(Event::PreLayout &)
            {
                _setMinimumSize(_p->border->getMinimumSize());
            }

            void Menu::_layoutEvent(Event::Layout &)
            {
                _p->border->setGeometry(getGeometry());
            }

            void Menu::_paintEvent(Event::Paint& event)
            {
                Widget::_paintEvent(event);
                if (auto render = _getRenderSystem().lock())
                {
                    if (auto style = _getStyle().lock())
                    {
                        const BBox2f & g = getGeometry();
                        if (_isHovered() || _p->checked)
                        {
                            render->setFillColor(_getColorWithOpacity(style->getColor(Style::ColorRole::Hover)));
                            render->drawRectangle(g);
                        }
                    }
                }
            }

            void Menu::_pointerEnterEvent(Event::PointerEnter& event)
            {
                event.accept();
                const auto id = event.getPointerInfo().id;
                _p->pointerHover[id] = true;
                _redraw();
            }

            void Menu::_pointerLeaveEvent(Event::PointerLeave& event)
            {
                event.accept();
                const auto id = event.getPointerInfo().id;
                const auto i = _p->pointerHover.find(id);
                if (i != _p->pointerHover.end())
                {
                    _p->pointerHover.erase(i);
                    _redraw();
                }
            }

            void Menu::_pointerMoveEvent(Event::PointerMove& event)
            {
                event.accept();
            }

            void Menu::_buttonPressEvent(Event::ButtonPress& event)
            {
                event.accept();
                setChecked(!_p->checked);
            }

            bool Menu::_isHovered() const
            {
                bool out = false;
                for (const auto& i : _p->pointerHover)
                {
                    out |= i.second;
                }
                return out;
            }

        } // namespace Button
    } // namespace UI
} // namespace Gp
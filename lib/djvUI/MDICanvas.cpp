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

#include <djvUI/MDICanvas.h>

#include <djvUI/Icon.h>
#include <djvUI/Label.h>
#include <djvUI/MDIWidget.h>
#include <djvUI/RowLayout.h>

#include <djvAV/Render2D.h>

#include <set>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace MDI
        {
            namespace
            {
                struct Hovered
                {
                    std::shared_ptr<IWidget> widget;
                    Handle handle = Handle::None;
                };

                struct Pressed
                {
                    std::shared_ptr<IWidget> widget;
                    glm::vec2 pointer = glm::vec2(0.f, 0.f);
                    Handle handle = Handle::None;
                    glm::vec2 pos = glm::vec2(0.f, 0.f);
                    glm::vec2 size = glm::vec2(0.f, 0.f);
                };

            } // namespace

            struct Canvas::Private
            {
                glm::vec2 canvasSize = glm::vec2(10000.f, 10000.f);
                std::map<std::shared_ptr<IWidget>, glm::vec2> widgetToPos;
                std::map<std::shared_ptr<IWidget>, bool> widgetResized;
                std::map<Event::PointerID, Hovered> hovered;
                std::map<Event::PointerID, Pressed> pressed;
                std::shared_ptr<IWidget> activeWidget;
                std::function<void(const std::shared_ptr<IWidget> &)> activeCallback;
            };

            void Canvas::_init(Context * context)
            {
                Widget::_init(context);

                setClassName("djv::UI::MDI::Canvas");
            }

            Canvas::Canvas() :
                _p(new Private)
            {}

            Canvas::~Canvas()
            {}

            std::shared_ptr<Canvas> Canvas::create(Context * context)
            {
                auto out = std::shared_ptr<Canvas>(new Canvas);
                out->_init(context);
                return out;
            }

            const glm::vec2 & Canvas::getCanvasSize() const
            {
                return _p->canvasSize;
            }

            void Canvas::setCanvasSize(const glm::vec2 & size)
            {
                DJV_PRIVATE_PTR();
                if (size == p.canvasSize)
                    return;
                p.canvasSize = size;
                _resize();
            }

            const std::shared_ptr<IWidget> & Canvas::getActiveWidget() const
            {
                return _p->activeWidget;
            }

            void Canvas::nextWidget()
            {
                const auto & children = getChildrenT<IWidget>();
                const size_t size = children.size();
                if (size > 1)
                {
                    children.back()->moveToBack();
                }
            }

            void Canvas::prevWidget()
            {
                const auto & children = getChildrenT<IWidget>();
                const size_t size = children.size();
                if (size > 1)
                {
                    children.front()->moveToFront();
                }
            }

            void Canvas::setActiveCallback(const std::function<void(const std::shared_ptr<IWidget> &)> & value)
            {
                _p->activeCallback = value;
            }

            glm::vec2 Canvas::getWidgetPos(const std::shared_ptr<IWidget> & widget) const
            {
                DJV_PRIVATE_PTR();
                const auto i = p.widgetToPos.find(widget);
                return i != p.widgetToPos.end() ? i->second : glm::vec2(0.f, 0.f);
            }

            void Canvas::setWidgetPos(const std::shared_ptr<IWidget> & widget, const glm::vec2 & pos)
            {
                DJV_PRIVATE_PTR();
                const auto i = p.widgetToPos.find(widget);
                if (i != p.widgetToPos.end())
                {
                    p.widgetToPos[widget] = pos;
                    _resize();
                }
            }

            void Canvas::_preLayoutEvent(Event::PreLayout &)
            {
                _setMinimumSize(_p->canvasSize);
            }

            void Canvas::_layoutEvent(Event::Layout &)
            {
                DJV_PRIVATE_PTR();
                const BBox2f & g = getGeometry();
                for (auto & i : p.widgetToPos)
                {
                    const glm::vec2& widgetMinimumSize = i.first->getMinimumSize();
                    i.second.x = Math::clamp(i.second.x, g.min.x, g.max.x - widgetMinimumSize.x);
                    i.second.y = Math::clamp(i.second.y, g.min.y, g.max.y - widgetMinimumSize.y);
                    if (i.first->isVisible())
                    {
                        const glm::vec2 & widgetSize = i.first->getSize();
                        BBox2f widgetGeometry;
                        widgetGeometry.min.x = g.min.x + i.second.x;
                        widgetGeometry.min.y = g.min.y + i.second.y;
                        widgetGeometry.max.x = Math::clamp(widgetGeometry.min.x + widgetSize.x, widgetGeometry.min.x + widgetMinimumSize.x, g.max.x);
                        widgetGeometry.max.y = Math::clamp(widgetGeometry.min.y + widgetSize.y, widgetGeometry.min.y + widgetMinimumSize.y, g.max.y);
                        i.first->setGeometry(widgetGeometry);
                    }
                }
            }

            void Canvas::_paintEvent(Event::Paint & event)
            {
                Widget::_paintEvent(event);
                auto style = _getStyle();
                const float h = style->getMetric(MetricsRole::Handle);
                const float sh = style->getMetric(MetricsRole::Shadow);
                auto render = _getRender();
                render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Shadow)));
                for (const auto & i : getChildrenT<IWidget>())
                {
                    if (i->isVisible())
                    {
                        BBox2f g = i->getGeometry().margin(-h);
                        g.min.x += sh;
                        g.min.y += sh;
                        g.max.x += sh;
                        g.max.y += sh;
                        render->drawShadow(g, sh);
                    }
                }
            }

            void Canvas::_paintOverlayEvent(Event::PaintOverlay & event)
            {
                DJV_PRIVATE_PTR();
                auto style = _getStyle();
                const float b = style->getMetric(MetricsRole::Border);
                const float h = style->getMetric(MetricsRole::Handle);

                auto render = _getRender();
                /*if (p.activeWidget && p.activeWidget->isVisible() && !p.activeWidget->isClipped())
                {
                    const BBox2f g = p.activeWidget->getGeometry().margin(-h);
                    render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Checked)));
                    render->drawRect(BBox2f(
                        glm::vec2(g.min.x, g.min.y),
                        glm::vec2(g.max.x, g.min.y + b)));
                    render->drawRect(BBox2f(
                        glm::vec2(g.min.x, g.max.y - b),
                        glm::vec2(g.max.x, g.max.y)));
                    render->drawRect(BBox2f(
                        glm::vec2(g.min.x, g.min.y + b),
                        glm::vec2(g.min.x + b, g.max.y - b)));
                    render->drawRect(BBox2f(
                        glm::vec2(g.max.x - b, g.min.y + b),
                        glm::vec2(g.max.x, g.max.y - b)));
                }*/

                auto hovered = p.hovered;
                render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Handle)));
                for (const auto & i : p.pressed)
                {
                    const auto& handles = i.second.widget->getHandlesDraw();
                    const auto j = handles.find(i.second.handle);
                    if (j != handles.end())
                    {
                        for (const auto& k : j->second)
                        {
                            switch (i.second.handle)
                            {
                            case Handle::Move:
                            case Handle::None: break;
                            default:
                                render->drawRect(k);
                                break;
                            }
                        }
                    }
                    const auto k = hovered.find(i.first);
                    if (k != hovered.end())
                    {
                        hovered.erase(k);
                    }
                }

                render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Handle)));
                for (const auto & i : hovered)
                {
                    const auto& handles = i.second.widget->getHandlesDraw();
                    const auto j = handles.find(i.second.handle);
                    if (j != handles.end())
                    {
                        for (const auto& k : j->second)
                        {
                            switch (i.second.handle)
                            {
                            case Handle::Move:
                            case Handle::None: break;
                            default:
                                render->drawRect(k);
                                break;
                            }
                        }
                    }
                }
            }

            void Canvas::_childAddedEvent(Event::ChildAdded & value)
            {
                DJV_PRIVATE_PTR();
                if (auto widget = std::dynamic_pointer_cast<IWidget>(value.getChild()))
                {
                    widget->installEventFilter(shared_from_this());
                    p.widgetToPos[widget] = glm::vec2(0.f, 0.f);
                    p.activeWidget = widget;
                    _resize();
                    if (p.activeCallback)
                    {
                        p.activeCallback(p.activeWidget);
                    }
                }
            }

            void Canvas::_childRemovedEvent(Event::ChildRemoved & value)
            {
                DJV_PRIVATE_PTR();
                if (auto widget = std::dynamic_pointer_cast<IWidget>(value.getChild()))
                {
                    widget->removeEventFilter(shared_from_this());
                    {
                        if (widget == p.activeWidget)
                        {
                            const auto & children = getChildrenT<IWidget>();
                            p.activeWidget = children.size() ? children.back() : nullptr;
                            if (p.activeCallback)
                            {
                                p.activeCallback(nullptr);
                            }
                        }
                    }
                    {
                        const auto j = p.widgetToPos.find(widget);
                        if (j != p.widgetToPos.end())
                        {
                            p.widgetToPos.erase(j);
                        }
                    }
                    _resize();
                }
            }

            void Canvas::_childOrderEvent(Core::Event::ChildOrder &)
            {
                DJV_PRIVATE_PTR();
                const auto & children = getChildrenT<IWidget>();
                if (children.size())
                {
                    auto widget = children.back();
                    if (widget != p.activeWidget)
                    {
                        p.activeWidget = widget;
                        if (p.activeCallback)
                        {
                            p.activeCallback(widget);
                        }
                    }
                }
            }

            bool Canvas::_eventFilter(const std::shared_ptr<IObject> & object, Event::IEvent & event)
            {
                DJV_PRIVATE_PTR();
                switch (event.getEventType())
                {
                case Event::Type::PointerEnter:
                {
                    Event::PointerEnter & pointerEnterEvent = static_cast<Event::PointerEnter &>(event);
                    const auto & pointerInfo = pointerEnterEvent.getPointerInfo();
                    if (auto widget = std::dynamic_pointer_cast<IWidget>(object))
                    {
                        for (const auto& handle : widget->getHandles())
                        {
                            for (const auto& rect : handle.second)
                            {
                                if (rect.contains(pointerInfo.projectedPos))
                                {
                                    event.accept();
                                    Hovered hovered;
                                    hovered.widget = widget;
                                    hovered.handle = handle.first;
                                    p.hovered[pointerInfo.id] = hovered;
                                    _redraw();
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case Event::Type::PointerLeave:
                {
                    Event::PointerLeave & pointerLeaveEvent = static_cast<Event::PointerLeave &>(event);
                    const auto & pointerInfo = pointerLeaveEvent.getPointerInfo();
                    const auto i = p.hovered.find(pointerInfo.id);
                    if (i != p.hovered.end())
                    {
                        event.accept();
                        p.hovered.erase(i);
                        _redraw();
                    }
                    break;
                }
                case Event::Type::PointerMove:
                {
                    event.accept();
                    Event::PointerMove & pointerMoveEvent = static_cast<Event::PointerMove &>(event);
                    const auto & pointerInfo = pointerMoveEvent.getPointerInfo();
                    if (auto widget = std::dynamic_pointer_cast<IWidget>(object))
                    {
                        const auto i = p.pressed.find(pointerInfo.id);
                        if (i != p.pressed.end())
                        {
                            const auto j = p.widgetToPos.find(widget);
                            if (j != p.widgetToPos.end())
                            {
                                glm::vec2 size = widget->getSize();
                                const glm::vec2 & minimumSize = widget->getMinimumSize();
                                const glm::vec2 d = pointerInfo.projectedPos - i->second.pointer;
                                const glm::vec2 d2(
                                    d.x - std::max(0.f, minimumSize.x - (i->second.size.x - d.x)),
                                    d.y - std::max(0.f, minimumSize.y - (i->second.size.y - d.y)));
                                switch (i->second.handle)
                                {
                                case Handle::Move:
                                    j->second = i->second.pos + pointerInfo.projectedPos - i->second.pointer;
                                    break;
                                case Handle::ResizeE:
                                {
                                    j->second.x = i->second.pos.x + d2.x;
                                    size.x = i->second.size.x - d2.x;
                                    break;
                                }
                                case Handle::ResizeN:
                                    j->second.y = i->second.pos.y + d2.y;
                                    size.y = i->second.size.y - d2.y;
                                    break;
                                case Handle::ResizeW:
                                    size.x = i->second.size.x + d.x;
                                    break;
                                case Handle::ResizeS:
                                    size.y = i->second.size.y + d.y;
                                    break;
                                case Handle::ResizeNE:
                                    j->second = i->second.pos + d2;
                                    size = i->second.size - d2;
                                    break;
                                case Handle::ResizeNW:
                                    j->second.y = i->second.pos.y + d2.y;
                                    size.x = i->second.size.x + d.x;
                                    size.y = i->second.size.y - d2.y;
                                    break;
                                case Handle::ResizeSW:
                                    size = i->second.size + pointerInfo.projectedPos - i->second.pointer;
                                    break;
                                case Handle::ResizeSE:
                                    j->second.x = i->second.pos.x + d2.x;
                                    size.x = i->second.size.x - d2.x;
                                    size.y = i->second.size.y + d.y;
                                    break;
                                default: break;
                                }
                                widget->resize(size);
                                _resize();
                            }
                        }
                        else
                        {
                            for (const auto& handle : widget->getHandles())
                            {
                                for (const auto& rect : handle.second)
                                {
                                    if (rect.contains(pointerInfo.projectedPos))
                                    {
                                        Hovered hovered;
                                        hovered.widget = widget;
                                        hovered.handle = handle.first;
                                        const auto j = p.hovered.find(pointerInfo.id);
                                        if (j != p.hovered.end() && (j->second.widget != widget || j->second.handle != handle.first))
                                        {
                                            p.hovered[pointerInfo.id] = hovered;
                                            _redraw();
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    return true;
                }
                case Event::Type::ButtonPress:
                {
                    Event::ButtonPress & buttonPressEvent = static_cast<Event::ButtonPress &>(event);
                    const auto & pointerInfo = buttonPressEvent.getPointerInfo();
                    if (auto widget = std::dynamic_pointer_cast<IWidget>(object))
                    {
                        const auto i = p.widgetToPos.find(widget);
                        if (i != p.widgetToPos.end())
                        {
                            for (const auto& handle : widget->getHandles())
                            {
                                for (const auto& rect : handle.second)
                                {
                                    if (rect.contains(pointerInfo.projectedPos))
                                    {
                                        event.accept();
                                        Pressed pressed;
                                        pressed.widget = widget;
                                        pressed.pointer = pointerInfo.projectedPos;
                                        pressed.handle = handle.first;
                                        pressed.pos = i->second;
                                        pressed.size = widget->getSize();
                                        p.pressed[pointerInfo.id] = pressed;
                                        widget->moveToFront();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    return true;
                }
                case Event::Type::ButtonRelease:
                {
                    Event::ButtonRelease & buttonReleaseEvent = static_cast<Event::ButtonRelease &>(event);
                    const auto & pointerInfo = buttonReleaseEvent.getPointerInfo();
                    const auto i = p.pressed.find(pointerInfo.id);
                    if (i != p.pressed.end())
                    {
                        event.accept();
                        p.pressed.erase(i);
                    }
                    return true;
                }
                default: break;
                }
                return false;
            }

        } // namespace MDI
    } // namespace UI
} // namespace djdv

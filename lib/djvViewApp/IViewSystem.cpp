// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvViewApp/IViewSystem.h>

#include <djvViewApp/InputSettings.h>
#include <djvViewApp/MDIWidget.h>

#include <djvUIComponents/UIComponentsSystem.h>

#include <djvUI/MDICanvas.h>
#include <djvUI/SettingsSystem.h>
#include <djvUI/Shortcut.h>

#include <djvCore/Context.h>
#include <djvCore/TextSystem.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        struct IViewSystem::Private
        {
            std::shared_ptr<UI::Settings::System> settingsSystem;
            std::shared_ptr<InputSettings> inputSettings;

            std::shared_ptr<UI::MDI::Canvas> canvas;
            std::map<std::string, std::shared_ptr<MDIWidget> > widgets;
            std::map<std::string, BBox2f> widgetGeom;

            std::shared_ptr<ValueObserver<bool> > textChangedObserver;
            std::shared_ptr<MapObserver<std::string, std::vector<UI::ShortcutData> > > shortcutsObserver;
        };

        void IViewSystem::_init(const std::string& name, const std::shared_ptr<Core::Context>& context)
        {
            ISystem::_init(name, context);
            DJV_PRIVATE_PTR();

            addDependency(context->getSystemT<UI::UIComponentsSystem>());

            p.settingsSystem = context->getSystemT<UI::Settings::System>();
            p.inputSettings = p.settingsSystem->getSettingsT<InputSettings>();

            auto weak = std::weak_ptr<IViewSystem>(std::dynamic_pointer_cast<IViewSystem>(shared_from_this()));
            p.textChangedObserver = ValueObserver<bool>::create(
                context->getSystemT<TextSystem>()->observeTextChanged(),
                [weak](bool value)
                {
                    if (value)
                    {
                        if (auto system = weak.lock())
                        {
                            system->_textUpdate();
                        }
                    }
                });

            p.shortcutsObserver = MapObserver<std::string, std::vector<UI::ShortcutData> >::create(
                p.inputSettings->observeShortcuts(),
                [weak](const std::map<std::string, std::vector<UI::ShortcutData> >& value)
                {
                    if (auto system = weak.lock())
                    {
                        system->_shortcutsUpdate();
                    }
                });
        }
            
        IViewSystem::IViewSystem() :
            _p(new Private)
        {}

        IViewSystem::~IViewSystem()
        {}
        
        std::map<std::string, std::shared_ptr<UI::Action> > IViewSystem::getActions() const
        {
            return std::map<std::string, std::shared_ptr<UI::Action> >();
        }

        MenuData IViewSystem::getMenu() const
        {
            return MenuData();
        }

        std::vector<std::shared_ptr<UI::ISettingsWidget> > IViewSystem::createSettingsWidgets() const
        {
            return std::vector<std::shared_ptr<UI::ISettingsWidget> >();
        }

        void IViewSystem::setCanvas(const std::shared_ptr<UI::MDI::Canvas>& value)
        {
            _p->canvas = value;
        }

        void IViewSystem::_openWidget(const std::string& name, const std::shared_ptr<MDIWidget>& widget)
        {
            DJV_PRIVATE_PTR();
            p.canvas->addChild(widget);
            p.widgets[name] = widget;
            auto weak = std::weak_ptr<IViewSystem>(std::dynamic_pointer_cast<IViewSystem>(shared_from_this()));
            widget->setCloseCallback(
                [weak, name]
                {
                    if (auto system = weak.lock())
                    {
                        system->_closeWidget(name);
                    }
                });
            const auto j = p.widgetGeom.find(name);
            if (j != p.widgetGeom.end())
            {
                p.canvas->setWidgetPos(widget, j->second.min);
                widget->resize(j->second.getSize());
            }
        }

        void IViewSystem::_closeWidget(const std::string& name)
        {
            DJV_PRIVATE_PTR();
            const auto i = p.widgets.find(name);
            if (i != p.widgets.end())
            {
                const glm::vec2& pos = p.canvas->getWidgetPos(i->second);
                const glm::vec2& size = i->second->getSize();
                p.widgetGeom[name] = BBox2f(pos.x, pos.y, size.x, size.y);
                p.canvas->removeChild(i->second);
                p.widgets.erase(i);
            }
        }

        const std::map<std::string, std::shared_ptr<MDIWidget> >& IViewSystem::_getWidgets() const
        {
            return _p->widgets;
        }

        const std::map<std::string, BBox2f>& IViewSystem::_getWidgetGeom() const
        {
            return _p->widgetGeom;
        }

        void IViewSystem::_setWidgetGeom(const std::map<std::string, BBox2f>& value)
        {
            _p->widgetGeom = value;
        }

        std::vector<UI::ShortcutData> IViewSystem::_getShortcuts(const std::string& value) const
        {
            const auto& shortcuts = _p->inputSettings->observeShortcuts()->get();
            const auto i = shortcuts.find(value);
            return i != shortcuts.end() ? i->second : std::vector<UI::ShortcutData>();
        }

        void IViewSystem::_addShortcut(const std::string& name, const std::vector<UI::ShortcutData>& value)
        {
            _p->inputSettings->addShortcut(name, value);
        }

        void IViewSystem::_addShortcut(const std::string& name, int key)
        {
            _p->inputSettings->addShortcut(name, key);
        }

        void IViewSystem::_addShortcut(const std::string& name, int key, int keyModifiers)
        {
            _p->inputSettings->addShortcut(name, key, keyModifiers);
        }

        void IViewSystem::_textUpdate()
        {}

        void IViewSystem::_shortcutsUpdate()
        {}

    } // namespace ViewApp
} // namespace djv

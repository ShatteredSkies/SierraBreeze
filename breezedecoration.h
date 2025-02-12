#ifndef BREEZE_DECORATION_H
#define BREEZE_DECORATION_H

/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
 * Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "breeze.h"
#include "breezesettings.h"

#include <KDecoration3/Decoration>
#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/DecorationSettings>

#include <QPalette>
#include <QPropertyAnimation>
#include <QVariant>
#include <QPainter>

namespace KDecoration3
{
    class DecorationButton;
    class DecorationButtonGroup;
}

namespace SierraBreeze
{
    class Decoration : public KDecoration3::Decoration
    {
        Q_OBJECT

        //* declare active state opacity
        Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

        public:

        //* constructor
        explicit Decoration(QObject *parent = nullptr, const QVariantList &args = QVariantList());

        //* destructor
        virtual ~Decoration();

        //* paint
        void paint(QPainter *painter, const QRectF &repaintRegion) override;

        //* internal settings
        InternalSettingsPtr internalSettings() const
        { return m_internalSettings; }

        //* caption height
        int captionHeight() const;

        //* button height
        int buttonHeight() const;

        //*@name active state change animation
        //@{
        void setOpacity( qreal );

        qreal opacity( void ) const
        { return m_opacity; }

        //@}

        //*@name colors
        //@{
        QColor titleBarColor( void ) const;
        QColor outlineColor( void ) const;
        QColor fontColor( void ) const;
        //@}

        //*@name maximization modes
        //@{
        inline bool isMaximized( void ) const;
        inline bool isMaximizedHorizontally( void ) const;
        inline bool isMaximizedVertically( void ) const;

        inline bool isLeftEdge( void ) const;
        inline bool isRightEdge( void ) const;
        inline bool isTopEdge( void ) const;
        inline bool isBottomEdge( void ) const;

        inline bool hideTitleBar( void ) const;
        inline bool matchColorForTitleBar( void ) const;
        //@}

        public Q_SLOTS:
        bool init() override;

        private Q_SLOTS:
        void reconfigure();
        void recalculateBorders();
        void updateButtonsGeometry();
        void updateButtonsGeometryDelayed();
        void updateTitleBar();
        void updateAnimationState();
        void updateBlur();

        private:

        //* return the rect in which caption will be drawn
        QPair<QRect,Qt::Alignment> captionRect( void ) const;

        void createButtons();
        void paintTitleBar(QPainter *painter, const QRectF &repaintRegion);
        void createShadow();

        void setScaledCornerRadius();

        //*@name border size
        //@{
        int borderSize(bool bottom = false) const;
        inline bool hasBorders( void ) const;
        inline bool hasNoBorders( void ) const;
        inline bool hasNoSideBorders( void ) const;
        //@}

        void setScaledTitleBarSideMargins();

        //*@name color customization
        //@{
        inline int titleBarAlpha() const;
        //@}

        InternalSettingsPtr m_internalSettings;
        QList<KDecoration3::DecorationButton*> m_buttons;
        KDecoration3::DecorationButtonGroup *m_leftButtons = nullptr;
        KDecoration3::DecorationButtonGroup *m_rightButtons = nullptr;

        //* active state change animation
        QPropertyAnimation *m_animation;

        //* active state change opacity
        qreal m_opacity = 0;

        //*frame corner radius, scaled according to DPI
        qreal m_scaledCornerRadius = 3;
        
        //* titleBar side margins, scaled according to smallspacing
        int m_scaledTitleBarLeftMargin = 1;
        int m_scaledTitleBarRightMargin = 1;

        //TODO Review this
        QPainter painter;
        const QRectF repaintRegion;

    };

    bool Decoration::hasBorders( void ) const
    {
        if( m_internalSettings && m_internalSettings->mask() & BorderSize ) return m_internalSettings->borderSize() > InternalSettings::BorderNoSides;
        else return settings()->borderSize() > KDecoration3::BorderSize::NoSides;
    }

    bool Decoration::hasNoBorders( void ) const
    {
        if( m_internalSettings && m_internalSettings->mask() & BorderSize ) return m_internalSettings->borderSize() == InternalSettings::BorderNone;
        else return settings()->borderSize() == KDecoration3::BorderSize::None;
    }

    bool Decoration::hasNoSideBorders( void ) const
    {
        if( m_internalSettings && m_internalSettings->mask() & BorderSize ) return m_internalSettings->borderSize() == InternalSettings::BorderNoSides;
        else return settings()->borderSize() == KDecoration3::BorderSize::NoSides;
    }

    bool Decoration::isMaximized( void ) const
    { return window()->isMaximized() && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isMaximizedHorizontally( void ) const
    { return window()->isMaximizedHorizontally() && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isMaximizedVertically( void ) const
    { return window()->isMaximizedVertically() && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isLeftEdge( void ) const
    { return (window()->isMaximizedHorizontally() || window()->adjacentScreenEdges().testFlag( Qt::LeftEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isRightEdge( void ) const
    { return (window()->isMaximizedHorizontally() || window()->adjacentScreenEdges().testFlag( Qt::RightEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isTopEdge( void ) const
    { return (window()->isMaximizedVertically() || window()->adjacentScreenEdges().testFlag( Qt::TopEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isBottomEdge( void ) const
    { return (window()->isMaximizedVertically() || window()->adjacentScreenEdges().testFlag( Qt::BottomEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::hideTitleBar( void ) const
    { return m_internalSettings->hideTitleBar() && !window()->isShaded(); }

    bool Decoration::matchColorForTitleBar( void ) const
    { return m_internalSettings->matchColorForTitleBar(); }

    int Decoration::titleBarAlpha() const
    {
        if (m_internalSettings->opaqueTitleBar())
            return 255;
        int a = m_internalSettings->opacityOverride() > -1 ? m_internalSettings->opacityOverride() : m_internalSettings->backgroundOpacity();
        a =  qBound(0, a, 100);
        return qRound(static_cast<qreal>(a) * static_cast<qreal>(2.55));
    }
}

#endif

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

#include "breezedecoration.h"

#include "breezesettingsprovider.h"

#include "breezebutton.h"

#include <KDecoration2/DecorationButtonGroup>
#include <KDecoration2/DecorationShadow>

#include <KColorUtils>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QPainter>
#include <QTextStream>
#include <QTimer>

#include <cmath>
#include <qnamespace.h>

K_PLUGIN_FACTORY_WITH_JSON(
    // BreezeDecoFactory,
    BreezeSierraDecoFactory,
    "breeze.json",
    registerPlugin<SierraBreeze::Decoration>();
    registerPlugin<SierraBreeze::Button>();
    //registerPlugin<SierraBreeze::ConfigWidget>();
)

namespace SierraBreeze
{

    using KDecoration2::ColorRole;
    using KDecoration2::ColorGroup;

    //________________________________________________________________
    static int g_sDecoCount = 0;
    static int g_shadowSize = 0;
    static int g_shadowStrength = 0;
    static QColor g_shadowColor = Qt::black;
    static std::shared_ptr<KDecoration2::DecorationShadow> g_sShadow;

    //________________________________________________________________
    Decoration::Decoration(QObject *parent, const QVariantList &args)
        : KDecoration2::Decoration(parent, args)
        , m_animation( new QPropertyAnimation( this ) )
    {
        g_sDecoCount++;
    }

    //________________________________________________________________
    Decoration::~Decoration()
    {
        g_sDecoCount--;
        if (g_sDecoCount == 0) {
            // last deco destroyed, clean up shadow
            g_sShadow.reset();
        }

    }

    //________________________________________________________________
    void Decoration::setOpacity( qreal value )
    {
        if( m_opacity == value ) return;
        m_opacity = value;
        update();
    }

    //________________________________________________________________
    QColor Decoration::titleBarColor() const
    {

        const auto c = client();
        if( hideTitleBar() ) return c->color( ColorGroup::Inactive, ColorRole::TitleBar );
        else if( m_animation->state() == QAbstractAnimation::Running )
        {
            return KColorUtils::mix(
                c->color( ColorGroup::Inactive, ColorRole::TitleBar ),
                c->color( ColorGroup::Active, ColorRole::TitleBar ),
                m_opacity );
        } else return c->color( c->isActive() ? ColorGroup::Active : ColorGroup::Inactive, ColorRole::TitleBar );

    }

    //________________________________________________________________
    QColor Decoration::outlineColor() const
    {

        auto c( client() );
        if( !m_internalSettings->drawTitleBarSeparator() ) return QColor();
        if( m_animation->state() == QPropertyAnimation::Running )
        {
            QColor color( c->palette().color( QPalette::Highlight ) );
            color.setAlpha( color.alpha()*m_opacity );
            return color;
        } else if( c->isActive() ) return c->palette().color( QPalette::Highlight );
        else return QColor();
    }

    //________________________________________________________________
    QColor Decoration::fontColor() const
    {

        auto c = client();
        if( m_animation->state() == QPropertyAnimation::Running )
        {
            return KColorUtils::mix(
                        c->color( ColorGroup::Inactive, ColorRole::Foreground ),
                        c->color( ColorGroup::Active, ColorRole::Foreground ),
                        m_opacity );
        } else {
                return  c->color( c->isActive() ? ColorGroup::Active : ColorGroup::Inactive, ColorRole::Foreground );
        }
    }

    //________________________________________________________________
    bool Decoration::init()
    {
        auto c = client();

        // active state change animation
        m_animation->setStartValue( 0 );
        m_animation->setEndValue( 1.0 );
        m_animation->setTargetObject( this );
        m_animation->setPropertyName( "opacity" );
        m_animation->setEasingCurve( QEasingCurve::InOutQuad );

        reconfigure();
        updateTitleBar();
        auto s = settings();
        connect(s.get(), &KDecoration2::DecorationSettings::borderSizeChanged, this, &Decoration::recalculateBorders);

        // a change in font might cause the borders to change
        connect(s.get(), &KDecoration2::DecorationSettings::fontChanged, this, &Decoration::recalculateBorders);
        connect(s.get(), &KDecoration2::DecorationSettings::spacingChanged, this, &Decoration::recalculateBorders);

        // buttons
        connect(s.get(), &KDecoration2::DecorationSettings::spacingChanged, this, &Decoration::updateButtonsGeometryDelayed);
        connect(s.get(), &KDecoration2::DecorationSettings::decorationButtonsLeftChanged, this, &Decoration::updateButtonsGeometryDelayed);
        connect(s.get(), &KDecoration2::DecorationSettings::decorationButtonsRightChanged, this, &Decoration::updateButtonsGeometryDelayed);

        // full reconfiguration
        connect(s.get(), &KDecoration2::DecorationSettings::reconfigured, this, &Decoration::reconfigure);
        connect(s.get(), &KDecoration2::DecorationSettings::reconfigured, SettingsProvider::self(), &SettingsProvider::reconfigure, Qt::UniqueConnection );
        connect(s.get(), &KDecoration2::DecorationSettings::reconfigured, this, &Decoration::updateButtonsGeometryDelayed);

        connect(c, &KDecoration2::DecoratedClient::adjacentScreenEdgesChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::maximizedHorizontallyChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::maximizedVerticallyChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::shadedChanged, this, &Decoration::recalculateBorders);
        connect(c, &KDecoration2::DecoratedClient::captionChanged, this,
           [this]()
           {
                // update the caption area
                update(titleBar());
           }
       );

        connect(c, &KDecoration2::DecoratedClient::activeChanged, this, &Decoration::updateAnimationState);
        connect(c, &KDecoration2::DecoratedClient::widthChanged, this, &Decoration::updateTitleBar);
        connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateTitleBar);
        //connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::setOpaque);

        connect(c, &KDecoration2::DecoratedClient::widthChanged, this, &Decoration::updateButtonsGeometry);
        connect(c, &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateButtonsGeometry);
        connect(c, &KDecoration2::DecoratedClient::adjacentScreenEdgesChanged, this, &Decoration::updateButtonsGeometry);
        connect(c, &KDecoration2::DecoratedClient::shadedChanged, this, &Decoration::updateButtonsGeometry);

        connect(s.get(), &KDecoration2::DecorationSettings::borderSizeChanged, this, &Decoration::updateBlur);
        connect(s.get(), &KDecoration2::DecorationSettings::fontChanged, this, &Decoration::updateBlur);
        connect(s.get(), &KDecoration2::DecorationSettings::spacingChanged, this, &Decoration::updateBlur);
        connect(c, &KDecoration2::DecoratedClient::activeChanged, this, &Decoration::updateBlur);
        connect(c, &KDecoration2::DecoratedClient::sizeChanged, this, &Decoration::updateBlur);

        createButtons();
        createShadow();

        return true;
    }

    //________________________________________________________________
    void Decoration::updateTitleBar()
    {
        const auto s = settings();
        const auto c = client();
        const bool maximized = isMaximized();
        const int width =  maximized ? c->width() : c->width() - 2*s->largeSpacing()*Metrics::TitleBar_SideMargin;
        const int height = maximized ? borderTop() : borderTop() - s->smallSpacing()*Metrics::TitleBar_TopMargin;
        const int x = maximized ? 0 : s->largeSpacing()*Metrics::TitleBar_SideMargin;
        const int y = maximized ? 0 : s->smallSpacing()*Metrics::TitleBar_TopMargin;
        setTitleBar(QRect(x, y, width, height));
    }

    //________________________________________________________________
    void Decoration::updateAnimationState()
    {
        if( m_internalSettings->animationsEnabled() )
        {

            const auto c = client();
            m_animation->setDirection( c->isActive() ? QPropertyAnimation::Forward : QPropertyAnimation::Backward );
            if( m_animation->state() != QPropertyAnimation::Running ) m_animation->start();

        } else {

            update();

        }
    }

    //________________________________________________________________
    void Decoration::updateSizeGripVisibility()
    {
        /*auto c = client();
        if( m_sizeGrip )
        { m_sizeGrip->setVisible( c->isResizeable() && !isMaximized() && !c->isShaded() ); }*/
    }

    //________________________________________________________________
    int Decoration::borderSize(bool bottom) const
    {
        const int baseSize = settings()->smallSpacing();
        if( m_internalSettings && (m_internalSettings->mask() & BorderSize ) )
        {
            switch (m_internalSettings->borderSize()) {
                case InternalSettings::BorderNone: return 0;
                case InternalSettings::BorderNoSides: return bottom ? qMax(4, baseSize) : 0;
                default:
                case InternalSettings::BorderTiny: return bottom ? qMax(4, baseSize) : baseSize;
                case InternalSettings::BorderNormal: return baseSize*2;
                case InternalSettings::BorderLarge: return baseSize*3;
                case InternalSettings::BorderVeryLarge: return baseSize*4;
                case InternalSettings::BorderHuge: return baseSize*5;
                case InternalSettings::BorderVeryHuge: return baseSize*6;
                case InternalSettings::BorderOversized: return baseSize*10;
            }

        } else {

            switch (settings()->borderSize()) {
                case KDecoration2::BorderSize::None: return 0;
                case KDecoration2::BorderSize::NoSides: return bottom ? qMax(4, baseSize) : 0;
                default:
                case KDecoration2::BorderSize::Tiny: return bottom ? qMax(4, baseSize) : baseSize;
                case KDecoration2::BorderSize::Normal: return baseSize*2;
                case KDecoration2::BorderSize::Large: return baseSize*3;
                case KDecoration2::BorderSize::VeryLarge: return baseSize*4;
                case KDecoration2::BorderSize::Huge: return baseSize*5;
                case KDecoration2::BorderSize::VeryHuge: return baseSize*6;
                case KDecoration2::BorderSize::Oversized: return baseSize*10;

            }

        }
    }

    //________________________________________________________________
    void Decoration::reconfigure()
    {

        m_internalSettings = SettingsProvider::self()->internalSettings( this );

        setScaledCornerRadius();

        // animation
        m_animation->setDuration( m_internalSettings->animationsDuration() );

        // borders
        recalculateBorders();

        // blur
        updateBlur();

        // shadow
        createShadow();
    }

    //________________________________________________________________
    void Decoration::recalculateBorders()
    {
        auto c = client();
        auto s = settings();

        // left, right and bottom borders
        const int left   = isLeftEdge() ? 0 : borderSize();
        const int right  = isRightEdge() ? 0 : borderSize();
        const int bottom = (c->isShaded() || isBottomEdge()) ? 0 : borderSize(true);

        int top = 0;
        if( hideTitleBar() ) top = bottom;
        else {

            QFontMetrics fm(s->font());
            top += qMax(fm.height(), buttonHeight() );

            // padding below
            // extra pixel is used for the active window outline
            const int baseSize = s->smallSpacing();
            top += baseSize*Metrics::TitleBar_BottomMargin + 1;

            // padding above
            top += baseSize*TitleBar_TopMargin;

        }

        setBorders(QMargins(left, top, right, bottom));

        // extended sizes
        const int extSize = s->largeSpacing();
        int extSides = 0;
        int extBottom = 0;
        if( hasNoBorders() )
        {
            extSides = extSize;
            extBottom = extSize;

        } else if( hasNoSideBorders() ) {

            extSides = extSize;

        }

        setResizeOnlyBorders(QMargins(extSides, 0, extSides, extBottom));
    }

    //________________________________________________________________
    void Decoration::updateBlur()
    {
        // NOTE: "BlurEffect::decorationBlurRegion()" will consider the intersection of
        // the blur and decoration regions. Here we need to focus on corner rounding.

        if (titleBarAlpha() == 255 || !settings()->isAlphaChannelSupported())
        { // no blurring without translucency
            setBlurRegion(QRegion());
            return;
        }

        QRegion region;
        const auto c = client();
        QSize rSize(m_scaledCornerRadius, m_scaledCornerRadius);

        if (!c->isShaded() && !isMaximized() && !hasNoBorders())
        {
            // exclude the titlebar
            int topBorder = hideTitleBar() ? 0 : borderTop();
            QRect rect(0, topBorder, size().width(), size().height() - topBorder);

            QRegion vert(QRect(rect.topLeft() + QPoint(m_scaledCornerRadius, 0),
                               QSize(rect.width() - 2*m_scaledCornerRadius, rect.height())));
            QRegion topLeft, topRight, bottomLeft, bottomRight, horiz;
            if (hasBorders())
            {
                if (hideTitleBar())
                {
                    topLeft = QRegion(QRect(rect.topLeft(), 2*rSize),
                                      isLeftEdge() ? QRegion::Rectangle : QRegion::Ellipse);
                    topRight = QRegion(QRect(rect.topLeft() + QPoint(rect.width() - 2*m_scaledCornerRadius, 0),
                                             2*rSize),
                                       isRightEdge() ? QRegion::Rectangle : QRegion::Ellipse);
                    horiz = QRegion(QRect(rect.topLeft() + QPoint(0, m_scaledCornerRadius),
                                          QSize(rect.width(), rect.height() - 2*m_scaledCornerRadius)));
                }
                else
                { // "horiz" is at the top because the titlebar is excluded
                    horiz = QRegion(QRect(rect.topLeft(),
                                    QSize(rect.width(), rect.height() - m_scaledCornerRadius)));
                }
                bottomLeft = QRegion(QRect(rect.topLeft() + QPoint(0, rect.height() - 2*m_scaledCornerRadius),
                                           2*rSize),
                                     isLeftEdge() && isBottomEdge() ? QRegion::Rectangle : QRegion::Ellipse);
                bottomRight = QRegion(QRect(rect.topLeft() + QPoint(rect.width() - 2*m_scaledCornerRadius,
                                                                    rect.height() - 2*m_scaledCornerRadius),
                                            2*rSize),
                                      isRightEdge() && isBottomEdge() ? QRegion::Rectangle : QRegion::Ellipse);
            }
            else // no side border
            {
                horiz = QRegion(QRect(rect.topLeft(),
                                      QSize(rect.width(), rect.height() - m_scaledCornerRadius)));
                bottomLeft = QRegion(QRect(rect.topLeft() + QPoint(0, rect.height() - 2*m_scaledCornerRadius),
                                           2*rSize),
                                     isBottomEdge() ? QRegion::Rectangle : QRegion::Ellipse);
                bottomRight = QRegion(QRect(rect.topLeft() + QPoint(rect.width() - 2*m_scaledCornerRadius,
                                                                    rect.height() - 2*m_scaledCornerRadius),
                                            2*rSize),
                                      isBottomEdge() ? QRegion::Rectangle : QRegion::Ellipse);
            }

            region = topLeft
                     .united(topRight)
                     .united(bottomLeft)
                     .united(bottomRight)
                     .united(horiz)
                     .united(vert);

            if (hideTitleBar())
            {
                setBlurRegion(region);
                return;
            }
        }

        const QRect titleRect(QPoint(0, 0), QSize(size().width(), borderTop()));

        // add the titlebar
        if (m_scaledCornerRadius == 0
            || isMaximized()) // maximized + no border when maximized
        {
            region |= QRegion(titleRect);
        }
        else if (c->isShaded())
        {
            QRegion topLeft(QRect(titleRect.topLeft(), 2*rSize), QRegion::Ellipse);
            QRegion topRight(QRect(titleRect.topLeft() + QPoint(titleRect.width() - 2*m_scaledCornerRadius, 0),
                                   2*rSize),
                             QRegion::Ellipse);
            QRegion bottomLeft(QRect(titleRect.topLeft() + QPoint(0, titleRect.height() - 2*m_scaledCornerRadius),
                                     2*rSize),
                               QRegion::Ellipse);
            QRegion bottomRight(QRect(titleRect.topLeft() + QPoint(titleRect.width() - 2*m_scaledCornerRadius,
                                                                   titleRect.height() - 2*m_scaledCornerRadius),
                                      2*rSize),
                                QRegion::Ellipse);
            region = topLeft
                     .united(topRight)
                     .united(bottomLeft)
                     .united(bottomRight)
                     // vertical
                     .united(QRect(titleRect.topLeft() + QPoint(m_scaledCornerRadius, 0),
                                   QSize(titleRect.width() - 2*m_scaledCornerRadius, titleRect.height())))
                     // horizontal
                     .united(QRect(titleRect.topLeft() + QPoint(0, m_scaledCornerRadius),
                                   QSize(titleRect.width(), titleRect.height() - 2*m_scaledCornerRadius)));
        }
        else
        {
            QRegion topLeft(QRect(titleRect.topLeft(), 2*rSize),
                            isLeftEdge() || isTopEdge() ? QRegion::Rectangle : QRegion::Ellipse);
            QRegion topRight(QRect(titleRect.topLeft() + QPoint(titleRect.width() - 2*m_scaledCornerRadius, 0),
                                   2*rSize),
                             isRightEdge() || isTopEdge() ? QRegion::Rectangle : QRegion::Ellipse);
            region |= topLeft
                      .united(topRight)
                      // vertical
                      .united(QRect(titleRect.topLeft() + QPoint(m_scaledCornerRadius, 0),
                                    QSize(titleRect.width() - 2*m_scaledCornerRadius, titleRect.height())))
                      // horizontal
                      .united(QRect(titleRect.topLeft() + QPoint(0, m_scaledCornerRadius),
                                    QSize(titleRect.width(), titleRect.height() - m_scaledCornerRadius)));
        }

        setBlurRegion(region);
    }

    //________________________________________________________________
void Decoration::createButtons()
    {
        m_leftButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Left, this, &Button::create);
        m_rightButtons = new KDecoration2::DecorationButtonGroup(KDecoration2::DecorationButtonGroup::Position::Right, this, &Button::create);
        updateButtonsGeometry();
    }

    //________________________________________________________________
    void Decoration::updateButtonsGeometryDelayed()
    { using namespace std::chrono_literals;
        QTimer::singleShot( 0ms, this, &Decoration::updateButtonsGeometry ); }

    //________________________________________________________________
    void Decoration::updateButtonsGeometry()
    {
        const auto s = settings();

        // adjust button position
        const int bHeight = captionHeight() + (isTopEdge() ? s->smallSpacing()*Metrics::TitleBar_TopMargin:0);
        const int bWidth = buttonHeight();
        const int verticalOffset = (isTopEdge() ? s->smallSpacing()*Metrics::TitleBar_TopMargin:0) + (captionHeight()-buttonHeight())/2;
        foreach( const QPointer<KDecoration2::DecorationButton>& button, m_leftButtons->buttons() + m_rightButtons->buttons() )
        {
            button.data()->setGeometry( QRectF( QPoint( 0, 0 ), QSizeF( bWidth, bHeight ) ) );
            static_cast<Button*>( button.data() )->setOffset( QPointF( 0, verticalOffset ) );
            static_cast<Button*>( button.data() )->setIconSize( QSize( bWidth, bWidth ) );
        }

        // left buttons
        if( !m_leftButtons->buttons().isEmpty() )
        {

            // spacing
            // m_leftButtons->setSpacing(s->smallSpacing()*Metrics::TitleBar_ButtonSpacing);
            // m_leftButtons->setSpacing(s->smallSpacing()*Metrics::TitleBar_ButtonSpacing);
            m_leftButtons->setSpacing(m_internalSettings->buttonSpacing());
            // m_leftButtons->setSpacing(s->largeSpacing()*Metrics::TitleBar_ButtonSpacing);

            // padding
            const int vPadding = isTopEdge() ? 0 : s->smallSpacing()*Metrics::TitleBar_TopMargin;
            // const int hPadding = s->smallSpacing()*Metrics::TitleBar_SideMargin;
            const int hPadding = m_internalSettings->buttonHPadding();
            if( isLeftEdge() )
            {
                // add offsets on the side buttons, to preserve padding, but satisfy Fitts law
                auto button = static_cast<Button*>( m_leftButtons->buttons().front() );
                button->setGeometry( QRectF( QPoint( 0, 0 ), QSizeF( bWidth + hPadding, bHeight ) ) );
                button->setFlag( Button::FlagFirstInList );
                button->setHorizontalOffset( hPadding );

                m_leftButtons->setPos(QPointF(0, vPadding));

            } else m_leftButtons->setPos(QPointF(hPadding + borderLeft(), vPadding));

        }

        // right buttons
        if( !m_rightButtons->buttons().isEmpty() )
        {

            // spacing
            // m_rightButtons->setSpacing(s->smallSpacing()*Metrics::TitleBar_ButtonSpacing);
            m_rightButtons->setSpacing(m_internalSettings->buttonSpacing());

            // padding
            const int vPadding = isTopEdge() ? 0 : s->smallSpacing()*Metrics::TitleBar_TopMargin;
            // const int hPadding = s->smallSpacing()*Metrics::TitleBar_SideMargin;
            const int hPadding = m_internalSettings->buttonHPadding();
            if( isRightEdge() )
            {

                auto button = static_cast<Button*>( m_rightButtons->buttons().back() );
                button->setGeometry( QRectF( QPoint( 0, 0 ), QSizeF( bWidth + hPadding, bHeight ) ) );
                button->setFlag( Button::FlagLastInList );

                m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width(), vPadding));

            } else m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width() - hPadding - borderRight(), vPadding));

        }

        update();

    }

    //________________________________________________________________
    void Decoration::paint(QPainter *painter, const QRect &repaintRegion)
    {
        // TODO: optimize based on repaintRegion
        auto c = client();
        auto s = settings();

        // paint background
        if( !c->isShaded() )
        {
            painter->fillRect(rect(), Qt::transparent);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);

            QColor winCol = this->titleBarColor();
            winCol.setAlpha(titleBarAlpha());
            painter->setBrush(winCol);

            // clip away the top part
            if( !hideTitleBar() ) painter->setClipRect(0, borderTop(), size().width(), size().height() - borderTop(), Qt::IntersectClip);

            if( s->isAlphaChannelSupported() ) painter->drawRoundedRect(rect(), Metrics::Frame_FrameRadius, Metrics::Frame_FrameRadius);
            else painter->drawRect( rect() );

            painter->restore();
        }

        if( !hideTitleBar() ) paintTitleBar(painter, repaintRegion);

        if( hasBorders() && !s->isAlphaChannelSupported() )
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setBrush( Qt::NoBrush );
            painter->setPen( c->isActive() ?
                c->color( ColorGroup::Active, ColorRole::TitleBar ):
                c->color( ColorGroup::Inactive, ColorRole::Foreground ) );

            painter->drawRect( rect().adjusted( 0, 0, -1, -1 ) );
            painter->restore();
        }

    }

    //________________________________________________________________
    void Decoration::paintTitleBar(QPainter *painter, const QRect &repaintRegion)
    {
        const auto c = client();
        // TODO Review this. Here the window color is appended in matchedTitleBarColor var
        const QColor matchedTitleBarColor(c->palette().color(QPalette::Window));
        const QRect titleRect(QPoint(0, 0), QSize(size().width(), borderTop()));

        if ( !titleRect.intersects(repaintRegion) ) return;

        painter->save();
        painter->setPen(Qt::NoPen);

        // render a linear gradient on title area
        if ( c->isActive() && m_internalSettings->drawBackgroundGradient() )
        {

            // TODO Review this. Initialize titleBarColor based on user's choise.
            QColor titleBarColor(matchColorForTitleBar()  ? matchedTitleBarColor : this->titleBarColor() );
            titleBarColor.setAlpha(titleBarAlpha());

            QLinearGradient gradient( 0, 0, 0, titleRect.height() );
            gradient.setColorAt(0.0, titleBarColor.lighter( 120 ) );
            gradient.setColorAt(0.8, titleBarColor);
            painter->setBrush(gradient);

        } else {

            // TODO Review this. Initialize titleBarColor based on user's choise.
            // I needed another else if because the window might not be active or has drawBackgroundGradient but
            QColor titleBarColor(matchColorForTitleBar()  ? matchedTitleBarColor : this->titleBarColor() );
            titleBarColor.setAlpha(titleBarAlpha());

            painter->setBrush(titleBarColor);

        }

        auto s = settings();
        if( isMaximized() || !s->isAlphaChannelSupported() )
        {

            painter->drawRect(titleRect);

        } else if( c->isShaded() ) {

            painter->drawRoundedRect(titleRect, Metrics::Frame_FrameRadius, Metrics::Frame_FrameRadius);

        } else {

            painter->setClipRect(titleRect, Qt::IntersectClip);

            // the rect is made a little bit larger to be able to clip away the rounded corners at the bottom and sides
            painter->drawRoundedRect(titleRect.adjusted(
                isLeftEdge() ? -Metrics::Frame_FrameRadius:0,
                isTopEdge() ? -Metrics::Frame_FrameRadius:0,
                isRightEdge() ? Metrics::Frame_FrameRadius:0,
                Metrics::Frame_FrameRadius),
                Metrics::Frame_FrameRadius, Metrics::Frame_FrameRadius);

        }

        const QColor outlineColor( this->outlineColor() );
        if( !c->isShaded() && outlineColor.isValid() )
        {
            // outline
            painter->setRenderHint( QPainter::Antialiasing, false );
            painter->setBrush( Qt::NoBrush );
            painter->setPen( outlineColor );
            painter->drawLine( titleRect.bottomLeft(), titleRect.bottomRight() );
        }

        painter->restore();

        // draw caption
        painter->setFont(s->font());
        painter->setPen( fontColor() );

        const auto cR = captionRect();
        const QString caption = painter->fontMetrics().elidedText(c->caption(), Qt::ElideMiddle, cR.first.width());
        painter->drawText(cR.first, cR.second | Qt::TextSingleLine, caption);

        // draw all buttons
        m_leftButtons->paint(painter, repaintRegion);
        m_rightButtons->paint(painter, repaintRegion);
    }

    //________________________________________________________________
    int Decoration::buttonHeight() const
    {
        const int baseSize = settings()->gridUnit();
        // const int modifier = m_internalSettings->buttonRadius();
        const int modifier = m_internalSettings->buttonSize();
        return baseSize + modifier;
    }

    //________________________________________________________________
    int Decoration::captionHeight() const
    { return hideTitleBar() ? borderTop() : borderTop() - settings()->smallSpacing()*(Metrics::TitleBar_BottomMargin + Metrics::TitleBar_TopMargin ) - 1; }

    //________________________________________________________________
    QPair<QRect,Qt::Alignment> Decoration::captionRect() const
    {
        if( hideTitleBar() ) return qMakePair( QRect(), Qt::AlignCenter );
        else {

            auto c = client();
            const int leftOffset = m_leftButtons->buttons().isEmpty() ?
                Metrics::TitleBar_SideMargin*settings()->smallSpacing():
                m_leftButtons->geometry().x() + m_leftButtons->geometry().width() + Metrics::TitleBar_SideMargin*settings()->smallSpacing();

            const int rightOffset = m_rightButtons->buttons().isEmpty() ?
                Metrics::TitleBar_SideMargin*settings()->smallSpacing() :
                size().width() - m_rightButtons->geometry().x() + Metrics::TitleBar_SideMargin*settings()->smallSpacing();

            const int yOffset = settings()->smallSpacing()*Metrics::TitleBar_TopMargin;
            const QRect maxRect( leftOffset, yOffset, size().width() - leftOffset - rightOffset, captionHeight() );

            switch( m_internalSettings->titleAlignment() )
            {
                case InternalSettings::AlignLeft:
                return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignLeft );

                case InternalSettings::AlignRight:
                return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignRight );

                case InternalSettings::AlignCenter:
                return qMakePair( maxRect, Qt::AlignCenter );

                default:
                case InternalSettings::AlignCenterFullWidth:
                {

                    // full caption rect
                    const QRect fullRect = QRect( 0, yOffset, size().width(), captionHeight() );
                    QRect boundingRect( settings()->fontMetrics().boundingRect( c->caption()).toRect() );

                    // text bounding rect
                    boundingRect.setTop( yOffset );
                    boundingRect.setHeight( captionHeight() );
                    boundingRect.moveLeft( ( size().width() - boundingRect.width() )/2 );

                    if( boundingRect.left() < leftOffset ) return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignLeft );
                    else if( boundingRect.right() > size().width() - rightOffset ) return qMakePair( maxRect, Qt::AlignVCenter|Qt::AlignRight );
                    else return qMakePair(fullRect, Qt::AlignCenter);

                }

            }

        }

    }

    //________________________________________________________________
    void Decoration::createShadow()
    {

        // assign global shadow if exists and parameters match
        if(
            !g_sShadow  ||
            g_shadowSize != m_internalSettings->shadowSize() ||
            g_shadowStrength != m_internalSettings->shadowStrength() ||
            g_shadowColor != m_internalSettings->shadowColor()
            )
        {
            // assign parameters
            g_shadowSize = m_internalSettings->shadowSize();
            g_shadowStrength = m_internalSettings->shadowStrength();
            g_shadowColor = m_internalSettings->shadowColor();
            const int shadowOffset = qMax( 6*g_shadowSize/16, Metrics::Shadow_Overlap*2 );

            // create image
            QImage image(2*g_shadowSize, 2*g_shadowSize, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);

            // create gradient
            // gaussian delta function
            auto alpha = [](qreal x) { return std::exp( -x*x/0.15 ); };

            // color calculation delta function
            auto gradientStopColor = [](QColor color, int alpha)
            {
                color.setAlpha(alpha);
                return color;
            };

            QRadialGradient radialGradient( g_shadowSize, g_shadowSize, g_shadowSize );
            for( int i = 0; i < 10; ++i )
            {
                const qreal x( qreal( i )/9 );
                radialGradient.setColorAt(x,  gradientStopColor( g_shadowColor, alpha(x)*g_shadowStrength ) );
            }

            radialGradient.setColorAt(1, gradientStopColor( g_shadowColor, 0 ) );

            // fill
            painter.begin(&image);
            //TODO review these
            //QPainter painter(&image);
            painter.setRenderHint( QPainter::Antialiasing, true );
            painter.fillRect( image.rect(), radialGradient);

            // contrast pixel
            QRectF innerRect = QRectF(
                g_shadowSize - Metrics::Shadow_Overlap, g_shadowSize - shadowOffset - Metrics::Shadow_Overlap,
                2*Metrics::Shadow_Overlap, shadowOffset + 2*Metrics::Shadow_Overlap );
                // g_shadowSize - shadowOffset - Metrics::Shadow_Overlap, g_shadowSize - shadowOffset - Metrics::Shadow_Overlap,
                // shadowOffset + 2*Metrics::Shadow_Overlap, shadowOffset + 2*Metrics::Shadow_Overlap );

            painter.setPen( gradientStopColor( g_shadowColor, g_shadowStrength*0.5 ) );
            painter.setBrush( Qt::NoBrush );
            painter.drawRoundedRect( innerRect, -0.5 + Metrics::Frame_FrameRadius, -0.5 + Metrics::Frame_FrameRadius );

            // mask out inner rect
            painter.setPen( Qt::NoPen );
            painter.setBrush( Qt::black );
            painter.setCompositionMode(QPainter::CompositionMode_DestinationOut );
            painter.drawRoundedRect( innerRect, 0.5 + Metrics::Frame_FrameRadius, 0.5 + Metrics::Frame_FrameRadius );

            painter.end();

            g_sShadow = std::make_shared<KDecoration2::DecorationShadow>();
            g_sShadow->setPadding( QMargins(
                // g_shadowSize - shadowOffset - Metrics::Shadow_Overlap,
                g_shadowSize - Metrics::Shadow_Overlap,
                g_shadowSize - shadowOffset - Metrics::Shadow_Overlap,
                g_shadowSize - Metrics::Shadow_Overlap,
                g_shadowSize - Metrics::Shadow_Overlap ) );

            g_sShadow->setInnerShadowRect(QRect( g_shadowSize, g_shadowSize, 1, 1) );

            // assign image
            g_sShadow->setShadow(image);

        }

        setShadow(g_sShadow);

    }

    void Decoration::setScaledCornerRadius()
    {
        m_scaledCornerRadius = m_internalSettings->cornerRadius() * settings()->smallSpacing();

    }

} // namespace


#include "breezedecoration.moc"

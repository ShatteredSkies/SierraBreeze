//////////////////////////////////////////////////////////////////////////////
// breezeconfigurationui.cpp
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "breezeconfigwidget.h"
#include "breezeexceptionlist.h"
#include "breezesettings.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>

namespace SierraBreeze
{

    //_________________________________________________________
    ConfigWidget::ConfigWidget( QObject* parent, const KPluginMetaData &data, const QVariantList &):
        KCModule(parent, data),
        m_configuration( KSharedConfig::openConfig( QStringLiteral( "sierrabreezerc" ) ) ),
        m_changed( false )
    {

        // configuration
        m_ui.setupUi(widget());

        // track ui changes
        connect( m_ui.titleAlignment, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        // connect( m_ui.buttonSize, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.outlineCloseButton, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.drawBorderOnMaximizedWindows, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.drawSizeGrip, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.opaqueTitleBar, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.drawBackgroundGradient, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.opacitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );
        connect( m_ui.drawTitleBarSeparator, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.matchColorForTitleBar, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect(m_ui.cornerRadius, SIGNAL(valueChanged(double)), SLOT(updateChanged()));

        connect( m_ui.buttonSize, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.buttonSpacing, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.buttonHPadding, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );

        connect( m_ui.buttonCloseActiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonCloseInactiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonMaximizeActiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonMaximizeInactiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonMinimizeActiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonMinimizeInactiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonOnAllDesktopsActiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonOnAllDesktopsInactiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonShadeActiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonShadeInactiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonKeepBelowActiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonKeepBelowInactiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonKeepAboveActiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );
        connect( m_ui.buttonKeepAboveInactiveColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );

        // track animations changes
        connect( m_ui.animationsEnabled, SIGNAL(clicked()), SLOT(updateChanged()) );
        connect( m_ui.animationsDuration, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );

        // track shadows changes
        connect( m_ui.shadowSize, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowStrength, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowColor, SIGNAL(changed(QColor)), SLOT(updateChanged()) );

        // track exception changes
        connect( m_ui.exceptions, SIGNAL(changed(bool)), SLOT(updateChanged()) );

    }

    //_________________________________________________________
    void ConfigWidget::load( void )
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->load();

        // assign to ui
        m_ui.titleAlignment->setCurrentIndex( m_internalSettings->titleAlignment() );
        // m_ui.buttonSize->setCurrentIndex( m_internalSettings->buttonSize() );
        m_ui.drawBorderOnMaximizedWindows->setChecked( m_internalSettings->drawBorderOnMaximizedWindows() );
        m_ui.outlineCloseButton->setChecked( m_internalSettings->outlineCloseButton() );
        m_ui.drawSizeGrip->setChecked( m_internalSettings->drawSizeGrip() );
        m_ui.opaqueTitleBar->setChecked( m_internalSettings->opaqueTitleBar() );
        m_ui.drawBackgroundGradient->setChecked( m_internalSettings->drawBackgroundGradient() );
        m_ui.animationsEnabled->setChecked( m_internalSettings->animationsEnabled() );
        m_ui.animationsDuration->setValue( m_internalSettings->animationsDuration() );
        m_ui.opacitySpinBox->setValue( m_internalSettings->backgroundOpacity() );
        m_ui.drawTitleBarSeparator->setChecked( m_internalSettings->drawTitleBarSeparator() );
        m_ui.buttonSize->setValue( m_internalSettings->buttonSize() );
        m_ui.buttonSpacing->setValue( m_internalSettings->buttonSpacing() );
        m_ui.buttonHPadding->setValue( m_internalSettings->buttonHPadding() );
        m_ui.matchColorForTitleBar->setChecked( m_internalSettings->matchColorForTitleBar() );
        m_ui.cornerRadius->setValue(m_internalSettings->cornerRadius());

        m_ui.buttonCloseActiveColor->setColor( m_internalSettings->buttonCloseActiveColor() );
        m_ui.buttonCloseInactiveColor->setColor( m_internalSettings->buttonCloseInactiveColor() );
        m_ui.buttonMaximizeActiveColor->setColor( m_internalSettings->buttonMaximizeActiveColor() );
        m_ui.buttonMaximizeInactiveColor->setColor( m_internalSettings->buttonMaximizeInactiveColor() );
        m_ui.buttonMinimizeActiveColor->setColor( m_internalSettings->buttonMinimizeActiveColor() );
        m_ui.buttonMinimizeInactiveColor->setColor( m_internalSettings->buttonMinimizeInactiveColor() );
        m_ui.buttonOnAllDesktopsActiveColor->setColor( m_internalSettings->buttonOnAllDesktopsActiveColor() );
        m_ui.buttonOnAllDesktopsInactiveColor->setColor( m_internalSettings->buttonOnAllDesktopsInactiveColor() );
        m_ui.buttonShadeActiveColor->setColor( m_internalSettings->buttonShadeActiveColor() );
        m_ui.buttonShadeInactiveColor->setColor( m_internalSettings->buttonShadeInactiveColor() );
        m_ui.buttonKeepBelowActiveColor->setColor( m_internalSettings->buttonKeepBelowActiveColor() );
        m_ui.buttonKeepBelowInactiveColor->setColor( m_internalSettings->buttonKeepBelowInactiveColor() );
        m_ui.buttonKeepAboveActiveColor->setColor( m_internalSettings->buttonKeepAboveActiveColor() );
        m_ui.buttonKeepAboveInactiveColor->setColor( m_internalSettings->buttonKeepAboveInactiveColor() );

        // load shadows
        m_ui.shadowSize->setValue( m_internalSettings->shadowSize() );
        m_ui.shadowStrength->setValue( qRound(qreal(m_internalSettings->shadowStrength()*100)/255 ) );
        m_ui.shadowColor->setColor( m_internalSettings->shadowColor() );

        // load exceptions
        ExceptionList exceptions;
        exceptions.readConfig( m_configuration );
        m_ui.exceptions->setExceptions( exceptions.get() );
        setChanged( false );

    }

    //_________________________________________________________
    void ConfigWidget::save( void )
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->load();

        // apply modifications from ui
        m_internalSettings->setTitleAlignment( m_ui.titleAlignment->currentIndex() );
        // m_internalSettings->setButtonSize( m_ui.buttonSize->currentIndex() );
        m_internalSettings->setOutlineCloseButton( m_ui.outlineCloseButton->isChecked() );
        m_internalSettings->setDrawBorderOnMaximizedWindows( m_ui.drawBorderOnMaximizedWindows->isChecked() );
        m_internalSettings->setDrawSizeGrip( m_ui.drawSizeGrip->isChecked() );
        m_internalSettings->setOpaqueTitleBar( m_ui.opaqueTitleBar->isChecked() );
        m_internalSettings->setDrawBackgroundGradient( m_ui.drawBackgroundGradient->isChecked() );
        m_internalSettings->setAnimationsEnabled( m_ui.animationsEnabled->isChecked() );
        m_internalSettings->setAnimationsDuration( m_ui.animationsDuration->value() );
        m_internalSettings->setBackgroundOpacity(m_ui.opacitySpinBox->value());
        m_internalSettings->setDrawTitleBarSeparator(m_ui.drawTitleBarSeparator->isChecked());
        m_internalSettings->setMatchColorForTitleBar( m_ui.matchColorForTitleBar->isChecked() );
        m_internalSettings->setCornerRadius(m_ui.cornerRadius->value());

        m_internalSettings->setButtonSize( m_ui.buttonSize->value() );
        m_internalSettings->setButtonSpacing( m_ui.buttonSpacing->value() );
        m_internalSettings->setButtonHPadding( m_ui.buttonHPadding->value() );

        m_internalSettings->setButtonCloseActiveColor( m_ui.buttonCloseActiveColor->color() );
        m_internalSettings->setButtonCloseInactiveColor( m_ui.buttonCloseInactiveColor->color() );
        m_internalSettings->setButtonMaximizeActiveColor( m_ui.buttonMaximizeActiveColor->color() );
        m_internalSettings->setButtonMaximizeInactiveColor( m_ui.buttonMaximizeInactiveColor->color() );
        m_internalSettings->setButtonMinimizeActiveColor( m_ui.buttonMinimizeActiveColor->color() );
        m_internalSettings->setButtonMinimizeInactiveColor( m_ui.buttonMinimizeInactiveColor->color() );
        m_internalSettings->setButtonOnAllDesktopsActiveColor( m_ui.buttonOnAllDesktopsActiveColor->color() );
        m_internalSettings->setButtonOnAllDesktopsInactiveColor( m_ui.buttonOnAllDesktopsInactiveColor->color() );
        m_internalSettings->setButtonShadeActiveColor( m_ui.buttonShadeActiveColor->color() );
        m_internalSettings->setButtonShadeInactiveColor( m_ui.buttonShadeInactiveColor->color() );
        m_internalSettings->setButtonKeepBelowActiveColor( m_ui.buttonKeepBelowActiveColor->color() );
        m_internalSettings->setButtonKeepBelowInactiveColor( m_ui.buttonKeepBelowInactiveColor->color() );
        m_internalSettings->setButtonKeepAboveActiveColor( m_ui.buttonKeepAboveActiveColor->color() );
        m_internalSettings->setButtonKeepAboveInactiveColor( m_ui.buttonKeepAboveInactiveColor->color() );

        m_internalSettings->setShadowSize( m_ui.shadowSize->value() );
        m_internalSettings->setShadowStrength( qRound( qreal(m_ui.shadowStrength->value()*255)/100 ) );
        m_internalSettings->setShadowColor( m_ui.shadowColor->color() );

        // save configuration
        m_internalSettings->save();

        // get list of exceptions and write
        InternalSettingsList exceptions( m_ui.exceptions->exceptions() );
        ExceptionList( exceptions ).writeConfig( m_configuration );

        // sync configuration
        m_configuration->sync();
        setChanged( false );

        // needed to tell kwin to reload when running from external kcmshell
        {
            QDBusMessage message = QDBusMessage::createSignal("/KWin", "org.kde.KWin", "reloadConfig");
            QDBusConnection::sessionBus().send(message);
        }

        // needed for breeze style to reload shadows
        {
            QDBusMessage message( QDBusMessage::createSignal("/BreezeDecoration",  "org.kde.Breeze.Style", "reparseConfiguration") );
            QDBusConnection::sessionBus().send(message);
        }

    }

    //_________________________________________________________
    void ConfigWidget::defaults( void )
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->setDefaults();

        // assign to ui
        m_ui.titleAlignment->setCurrentIndex( m_internalSettings->titleAlignment() );
        // m_ui.buttonSize->setCurrentIndex( m_internalSettings->buttonSize() );
        m_ui.drawBorderOnMaximizedWindows->setChecked( m_internalSettings->drawBorderOnMaximizedWindows() );
        m_ui.drawSizeGrip->setChecked( m_internalSettings->drawSizeGrip() );
        m_ui.opaqueTitleBar->setChecked( m_internalSettings->opaqueTitleBar() );
        m_ui.drawBackgroundGradient->setChecked( m_internalSettings->drawBackgroundGradient() );
        m_ui.animationsEnabled->setChecked( m_internalSettings->animationsEnabled() );
        m_ui.animationsDuration->setValue( m_internalSettings->animationsDuration() );
        m_ui.opacitySpinBox->setValue( m_internalSettings->backgroundOpacity() );
        m_ui.drawTitleBarSeparator->setChecked( m_internalSettings->drawTitleBarSeparator() );
        m_ui.cornerRadius->setValue(m_internalSettings->cornerRadius());

        m_ui.buttonSize->setValue( m_internalSettings->buttonSize() );
        m_ui.buttonSpacing->setValue( m_internalSettings->buttonSpacing() );
        m_ui.buttonHPadding->setValue( m_internalSettings->buttonHPadding() );

        m_ui.buttonCloseActiveColor->setColor( m_internalSettings->buttonCloseActiveColor() );
        m_ui.buttonCloseInactiveColor->setColor( m_internalSettings->buttonCloseInactiveColor() );
        m_ui.buttonMaximizeActiveColor->setColor( m_internalSettings->buttonMaximizeActiveColor() );
        m_ui.buttonMaximizeInactiveColor->setColor( m_internalSettings->buttonMaximizeInactiveColor() );
        m_ui.buttonMinimizeActiveColor->setColor( m_internalSettings->buttonMinimizeActiveColor() );
        m_ui.buttonMinimizeInactiveColor->setColor( m_internalSettings->buttonMinimizeInactiveColor() );
        m_ui.buttonOnAllDesktopsActiveColor->setColor( m_internalSettings->buttonOnAllDesktopsActiveColor() );
        m_ui.buttonOnAllDesktopsInactiveColor->setColor( m_internalSettings->buttonOnAllDesktopsInactiveColor() );
        m_ui.buttonShadeActiveColor->setColor( m_internalSettings->buttonShadeActiveColor() );
        m_ui.buttonShadeInactiveColor->setColor( m_internalSettings->buttonShadeInactiveColor() );
        m_ui.buttonKeepBelowActiveColor->setColor( m_internalSettings->buttonKeepBelowActiveColor() );
        m_ui.buttonKeepBelowInactiveColor->setColor( m_internalSettings->buttonKeepBelowInactiveColor() );
        m_ui.buttonKeepAboveActiveColor->setColor( m_internalSettings->buttonKeepAboveActiveColor() );
        m_ui.buttonKeepAboveInactiveColor->setColor( m_internalSettings->buttonKeepAboveInactiveColor() );

        m_ui.shadowSize->setValue( m_internalSettings->shadowSize() );
        m_ui.shadowStrength->setValue( qRound(qreal(m_internalSettings->shadowStrength()*100)/255 ) );
        m_ui.shadowColor->setColor( m_internalSettings->shadowColor() );
        m_ui.matchColorForTitleBar->setChecked( m_internalSettings->matchColorForTitleBar() );

    }

    //_______________________________________________
    void ConfigWidget::updateChanged( void )
    {

        // check configuration
        if( !m_internalSettings ) return;

        // track modifications
        bool modified( false );

        if (m_ui.drawTitleBarSeparator->isChecked() != m_internalSettings->drawTitleBarSeparator()) modified = true;
        else if( m_ui.titleAlignment->currentIndex() != m_internalSettings->titleAlignment() ) modified = true;
        // else if( m_ui.buttonSize->currentIndex() != m_internalSettings->buttonSize() ) modified = true;
        else if( m_ui.outlineCloseButton->isChecked() != m_internalSettings->outlineCloseButton() ) modified = true;
        else if( m_ui.drawBorderOnMaximizedWindows->isChecked() !=  m_internalSettings->drawBorderOnMaximizedWindows() ) modified = true;
        else if( m_ui.drawSizeGrip->isChecked() !=  m_internalSettings->drawSizeGrip() ) modified = true;
        else if( m_ui.opaqueTitleBar->isChecked() !=  m_internalSettings->opaqueTitleBar() ) modified = true;
        else if( m_ui.drawBackgroundGradient->isChecked() !=  m_internalSettings->drawBackgroundGradient() ) modified = true;
        else if (m_ui.cornerRadius->value() != m_internalSettings->cornerRadius()) modified = true;
        else if( m_ui.buttonSize->value() != m_internalSettings->buttonSize() ) modified = true;
        else if( m_ui.buttonSpacing->value() != m_internalSettings->buttonSpacing() ) modified = true;
        else if( m_ui.buttonHPadding->value() != m_internalSettings->buttonHPadding() ) modified = true;
        else if( m_ui.buttonCloseActiveColor->color() != m_internalSettings->buttonCloseActiveColor() ) modified = true;
        else if( m_ui.buttonCloseInactiveColor->color() != m_internalSettings->buttonCloseInactiveColor() ) modified = true;
        else if( m_ui.buttonMaximizeActiveColor->color() != m_internalSettings->buttonMaximizeActiveColor() ) modified = true;
        else if( m_ui.buttonMaximizeInactiveColor->color() != m_internalSettings->buttonMaximizeInactiveColor() ) modified = true;
        else if( m_ui.buttonMinimizeActiveColor->color() != m_internalSettings->buttonMinimizeActiveColor() ) modified = true;
        else if( m_ui.buttonMinimizeInactiveColor->color() != m_internalSettings->buttonMinimizeInactiveColor() ) modified = true;
        else if( m_ui.buttonOnAllDesktopsActiveColor->color() != m_internalSettings->buttonOnAllDesktopsActiveColor() ) modified = true;
        else if( m_ui.buttonOnAllDesktopsInactiveColor->color() != m_internalSettings->buttonOnAllDesktopsInactiveColor() ) modified = true;
        else if( m_ui.buttonShadeActiveColor->color() != m_internalSettings->buttonShadeActiveColor() ) modified = true;
        else if( m_ui.buttonShadeInactiveColor->color() != m_internalSettings->buttonShadeInactiveColor() ) modified = true;
        else if( m_ui.buttonKeepBelowActiveColor->color() != m_internalSettings->buttonKeepBelowActiveColor() ) modified = true;
        else if( m_ui.buttonKeepBelowInactiveColor->color() != m_internalSettings->buttonKeepBelowInactiveColor() ) modified = true;
        else if( m_ui.buttonKeepAboveActiveColor->color() != m_internalSettings->buttonKeepAboveActiveColor() ) modified = true;
        else if( m_ui.buttonKeepAboveInactiveColor->color() != m_internalSettings->buttonKeepAboveInactiveColor() ) modified = true;
        else if( m_ui.opacitySpinBox->value() != m_internalSettings->backgroundOpacity() ) modified = true;

        // animations
        else if( m_ui.animationsEnabled->isChecked() !=  m_internalSettings->animationsEnabled() ) modified = true;
        else if( m_ui.animationsDuration->value() != m_internalSettings->animationsDuration() ) modified = true;

        // shadows
        else if( m_ui.shadowSize->value() !=  m_internalSettings->shadowSize() ) modified = true;
        else if( qRound( qreal(m_ui.shadowStrength->value()*255)/100 ) != m_internalSettings->shadowStrength() ) modified = true;
        else if( m_ui.shadowColor->color() != m_internalSettings->shadowColor() ) modified = true;

        // exceptions
        else if( m_ui.exceptions->isChanged() ) modified = true;

        // TODO Review this
        // colors of title bar and window background
        else if( m_ui.matchColorForTitleBar->isChecked() != m_internalSettings->matchColorForTitleBar() ) modified = true;
        setChanged( modified );

    }

    //_______________________________________________
    void ConfigWidget::setChanged( bool value )
    {
        setNeedsSave( value );
    }

}

#ifndef APP_MAC_HPP
#define APP_MAC_HPP
/*
 * QEstEidCommon
 *
 * Copyright (C) 2010 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2010 Raul Metsma <raul@innovaatik.ee>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <QObject>

#ifdef Q_OS_MAC

class AppMac: public QObject {
    Q_OBJECT
public:
    AppMac( QObject *parent = 0 );
    ~AppMac();
    auto setAlwaysOnTop(QWidget *widget, bool onTop) -> void;
    auto devices() const -> QStringList;
    auto setScreensaverDisabled(bool disabled) -> void;
    auto shutdown() -> bool;
private:
    auto eventFilter( QObject *o, QEvent *e ) -> bool;
    struct Data;
    Data *d;
};

#endif

#endif

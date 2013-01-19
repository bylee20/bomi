/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project on Qt Labs.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions contained
** in the Technology Preview License Agreement accompanying this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
****************************************************************************/

#ifndef QWINDOWWIDGET_H
#define QWINDOWWIDGET_H

#include "stdafx.hpp"
//
// QWindowWidget supports embedding a QWindow in a QWidget hierarchy,
// making the QWindow geometry track the QWidget geometry.
//
// This class has one major limitation: the embedded QWindow has a
// native (non-toplevel) window surface which is overlayed the QWidget
// hierarchy. This means that "complex" configurations like placing
// placing the QWindowWidget in a scroll area will most likely not work.
//
class QWindowFrame : public QFrame
{
public:
	QWindowFrame(QWidget *parent = nullptr);
	~QWindowFrame();
	void setEmbeddedWindow(QWindow *embeddedWindow);
	QWindow *embeddedWindow() const;
	bool event(QEvent *event);
private:
	QWindow *m_EmbeddedWindow;
//	QWidget *m_top;
};

#endif // QWIDGETWINDOW_H

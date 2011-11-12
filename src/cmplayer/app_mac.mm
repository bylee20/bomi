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

#include "application_mac.hpp"
#include <QtCore/QAbstractEventDispatcher>
#ifdef Q_WS_MAC

#include <QtCore/QDebug>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include "events.hpp"
#include <QtGui/QApplication>
#include <Cocoa/Cocoa.h>
#include <mach/mach_port.h>
#include <sys/param.h>
#include <paths.h>
#include <IOKit/storage/IODVDMedia.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/IOBSD.h>

@interface ApplicationObjC : NSObject {
}

- (id) init;
- (void) appReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
@end

@implementation ApplicationObjC
- (id) init
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	[super init];
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
		andSelector:@selector(appReopen:withReplyEvent:)
		forEventClass:kCoreEventClass
		andEventID:kAEReopenApplication];
	[pool release];
	return self;
}

- (void) dealloc
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	[[NSAppleEventManager sharedAppleEventManager] removeEventHandlerForEventClass:kCoreEventClass
		andEventID:kAEReopenApplication];
	[pool release];
	[super dealloc];
}

- (void) appReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
	Q_UNUSED(event);
	Q_UNUSED(replyEvent);
	QApplication::postEvent( qApp, new ReopenEvent );
}
@end

struct ApplicationMacData {
	ApplicationObjC *objc;
	bool eventsLoaded;
};

ApplicationMac::ApplicationMac( QObject *parent )
: QObject( parent )
, d( new ApplicationMacData ) {
	d->eventsLoaded = false;
	d->objc = [[ApplicationObjC alloc] init];
	qApp->installEventFilter( this );
}

ApplicationMac::~ApplicationMac() {
	[d->objc release];
	delete d;
}

static void mac_install_event_handler(QObject *app) {
	new ApplicationMac( app );
}

bool ApplicationMac::eventFilter( QObject *o, QEvent *e ) {
	// Load here because cocoa NSApplication overides events
	if(!d->eventsLoaded && o == qApp && e->type() == QEvent::ApplicationActivate) {
		mac_install_event_handler(this);
		d->eventsLoaded = true;
	}
	return QObject::eventFilter( o, e );
}

void ApplicationMac::setAlwaysOnTop(WId wid, bool onTop) {
	NSView *view = (NSView*)(void*)wid;
	NSWindow *window = [view window];
	if (onTop)
		[window setLevel:NSFloatingWindowLevel];
	else
		[window setLevel:NSNormalWindowLevel];
}

QStringList ApplicationMac::devices() const {
	mach_port_t port;
	kern_return_t kernRet = IOMasterPort(MACH_PORT_NULL, &port);
	if (kernRet != KERN_SUCCESS)
		return QStringList();
	CFMutableDictionaryRef dict = IOServiceMatching(kIODVDMediaClass);
	if (!dict)
		return QStringList();
	CFDictionarySetValue(dict, CFSTR(kIOMediaEjectableKey), kCFBooleanTrue);
	io_iterator_t it;
	kernRet = IOServiceGetMatchingServices(port, dict, &it);
	if (kernRet != KERN_SUCCESS)
		return QStringList();
	io_object_t device = 0;
	QList<QString> devices;
	while ((device = IOIteratorNext(it))) {
		CFStringRef name = reinterpret_cast<CFStringRef>(
				IORegistryEntryCreateCFProperty(device
					, CFSTR(kIOBSDNameKey), kCFAllocatorDefault, 0));
		if (!name) {
			IOObjectRelease(device);
			continue;
		}
		char path[MAXPATHLEN] = {0};
		sprintf(path, "%sr", _PATH_DEV);
		const size_t len = qstrlen(path);
		if (CFStringGetCString(name, path + len, sizeof(path) - len, kCFStringEncodingASCII))
			devices.push_back(QLatin1String(path));
		CFRelease(name);
		IOObjectRelease(device);
	}
	IOObjectRelease(it);
	return devices;
}

void ApplicationMac::setScreensaverDisabled(bool disabled) {
	static bool prev = false;
	static IOPMAssertionID idle = 0;
	static IOPMAssertionID display = 0;
	if (prev == disabled)
		return;
	prev = disabled;
	if (disabled) {
		IOPMAssertionCreate(kIOPMAssertionTypeNoIdleSleep, kIOPMAssertionLevelOn, &idle);
		IOPMAssertionCreate(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, &display);
	} else {
		IOPMAssertionRelease(idle);
		IOPMAssertionRelease(display);
		idle = display = 0;
	}
}

QString ApplicationMac::test() {
	int i; // Loop counter.

	// Create the File Open Dialog class.
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];

	// Enable the selection of files in the dialog.
	[openDlg setCanChooseFiles:YES];

	// Enable the selection of directories in the dialog.
	[openDlg setCanChooseDirectories:YES];

	// Display the dialog.  If the OK button was pressed,
	// process the files.
	int ret = [openDlg runModalForDirectory:nil file:nil];
	QAbstractEventDispatcher::instance()->interrupt();
	if (ret== NSOKButton )
	{
	    // Get an array containing the full filenames of all
	    // files and directories selected.
	    NSArray* files = [openDlg filenames];

	    // Loop through all the files and process them.
	    for( i = 0; i < (int)[files count]; i++ )
	    {
		NSString* filename = [files objectAtIndex:i];
		NSRange range;
		range.location = 0;
		range.length = [filename length];
    //	    QString result(range.length, QChar(0));

		unichar *chars = new unichar[range.length];
		[filename getCharacters:chars range:range];
		QString result = QString::fromUtf16(chars, range.length);
		delete[] chars;
		return result;
		// Do something with the filename.
	    }
	}

	return QString();


	NSOpenPanel *op = [NSOpenPanel openPanel];
	if ([op runModal] == NSOKButton)
	{
	    NSString *filename = [op filename];
	    NSRange range;
	    range.location = 0;
	    range.length = [filename length];
//	    QString result(range.length, QChar(0));

	    unichar *chars = new unichar[range.length];
	    [filename getCharacters:chars range:range];
	    QString result = QString::fromUtf16(chars, range.length);
	    delete[] chars;
	    return result;
	}
	return QString();
}

#endif

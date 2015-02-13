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

#include "app_mac.hpp"

#ifdef Q_OS_MAC

#include <QEvent>
#include <QApplication>
#include <QWidget>
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <Cocoa/Cocoa.h>
#include <mach/mach_port.h>
#include <sys/param.h>
#include <paths.h>
#include <IOKit/storage/IODVDMedia.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/IOBSD.h>

static const int ReopenEvent = QEvent::User + 1;

@interface AppObjC : NSObject {
}

- (id) init;
- (void) appReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
@end

@implementation AppObjC
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
	QApplication::postEvent( qApp, new QEvent((QEvent::Type)ReopenEvent));
}
@end

struct AppMac::Data {
	AppObjC *objc;
	bool eventsLoaded;
	IOPMAssertionID aid = 0;
	CFMutableDictionaryRef prop = 0;
	CFNumberRef on = 0, off = 0;
};

AppMac::AppMac(QObject *parent)
: QObject(parent), d(new Data) {
	d->eventsLoaded = false;
	d->objc = [[AppObjC alloc] init];
	qApp->installEventFilter( this );

//	static const int AssertionOn = kIOPMAssertionLevelOn;
//	static const int AssertionOff = kIOPMAssertionLevelOff;

//	d->on = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &AssertionOn);
//	d->off = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &AssertionOff);
//	d->prop = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
//	if (d->on && d->off && d->prop) {
//		CFDictionarySetValue(d->prop, kIOPMAssertionTypePreventUserIdleSystemSleep, d->off);
//		CFDictionarySetValue(d->prop, kIOPMAssertionTypePreventUserIdleDisplaySleep, d->off);
//		CFDictionarySetValue(d->prop, kIOPMAssertionTypePreventSystemSleep, d->off);
//		CFDictionarySetValue(d->prop, kIOPMAssertionNameKey, CFSTR("bomi"));
//		if (IOPMAssertionCreateWithProperties(d->prop, &d->aid) != kIOReturnSuccess)
//			d->aid = 0;
//	}
//	if (d->prop)
//		CFRelease(d->prop);
}

AppMac::~AppMac() {
	if (d->aid)
		IOPMAssertionRelease(d->aid);
	if (d->on)
		CFRelease(d->on);
	if (d->off)
		CFRelease(d->off);
	[d->objc release];
	delete d;
}

static void mac_install_event_handler(QObject *app) {
	new AppMac( app );
}

bool AppMac::eventFilter( QObject *o, QEvent *e ) {
	// Load here because cocoa NSApplication overides events
	if(!d->eventsLoaded && o == qApp && e->type() == QEvent::ApplicationActivate) {
		mac_install_event_handler(this);
		d->eventsLoaded = true;
	}
    return QObject::eventFilter( o, e );
}

void AppMac::setAlwaysOnTop(QWidget *widget, bool onTop) {
	NSView *view = (NSView*)(void*)widget->effectiveWinId();
	NSWindow *window = [view window];
	if (onTop)
		[window setLevel:NSFloatingWindowLevel];
	else
		[window setLevel:NSNormalWindowLevel];
}

QStringList AppMac::devices() const {
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
    QStringList devices;
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
            devices.push_back(QString::fromLatin1(path));
		CFRelease(name);
		IOObjectRelease(device);
	}
	IOObjectRelease(it);
	return devices;
}

void AppMac::setScreensaverDisabled(bool disabled) {
	if (disabled == (d->aid != 0))
		return;
	if (d->aid) {
		IOPMAssertionRelease(d->aid);
		d->aid = 0;
	}
	if (disabled)
		IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep
			, kIOPMAssertionLevelOn, CFSTR("net.xylosper.bomi.playing"), &d->aid);
//	const CFNumberRef level = disabled ? d->on : d->off;
//	IOPMAssertionSetProperty(d->aid, kIOPMAssertionTypePreventUserIdleSystemSleep, level);
//	IOPMAssertionSetProperty(d->aid, kIOPMAssertionTypePreventUserIdleDisplaySleep, level);
//	IOPMAssertionSetProperty(d->aid, kIOPMAssertionTypePreventSystemSleep, level);
}

static OSStatus sendAE(AEEventID EventToSend) {
	OSStatus status = noErr;
	do {
		AEAddressDesc desc;
		static const ProcessSerialNumber psn = { 0, kSystemProcess };
		OSStatus status = AECreateDesc(typeProcessSerialNumber, &psn, sizeof(psn), &desc);
		if (status != noErr)
			break;
		AppleEvent event = {typeNull, NULL};
		status = AECreateAppleEvent(kCoreEventClass, EventToSend, &desc, kAutoGenerateReturnID, kAnyTransactionID, &event);
		AEDisposeDesc(&desc);
		if (status != noErr)
			break;
		AppleEvent reply = {typeNull, NULL};
		status = AESend(&event, &reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
		AEDisposeDesc(&event);
		if (status != noErr)
			break;
		AEDisposeDesc(&reply);

	} while (false);
	return status;
}

bool AppMac::shutdown() {
	return sendAE(kAEShutDown) != noErr;
}

#if 0

// keep

auto MainWindow::setFullScreen(bool full) -> void
{
    d->dontPause = true;
    if (isFullScreen() != full) {
#ifdef Q_OS_MAC
        if (!d->pref.lion_style_fullscreen()) {
            static Qt::WindowFlags flags = windowFlags();
            static QRect geometry;
            if (full) {
                auto desktop = cApp.desktop();
                const int screen = desktop->screenNumber(this);
                if (screen >= 0) {
                    flags = windowFlags();
                    geometry = this->geometry();
                    setWindowFlags(flags | Qt::FramelessWindowHint);
                    SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
                    show();
                    setGeometry(QRect(QPoint(0, 0),
                                      desktop->screenGeometry(this).size()));
                }
            } else {
                setWindowFlags(flags);
                setGeometry(geometry);
                SetSystemUIMode(kUIModeNormal, 0);
            }
            d->checkWindowState(d->winState);
            d->updateTitle();
            d->updateStaysOnTop();
        } else
#endif
        {
            OS::setFullScreen(this, full);
//            if (!full) {
//                if (d->prevWinState & Qt::WindowMaximized)
//                    showMaximized();
//                else
//                    showNormal();
//            }
        }
    }
    d->dontPause = false;
}

#endif

#endif


#ifdef Q_OS_MAC
#include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#include <libproc.h>
QString UtilObject::monospace() { return u"monaco"_q; }
template<class T>
static T getSysctl(int name, const T def) {
    T ret; int names[] = {CTL_HW, name}; size_t len = sizeof(def);
    return (sysctl(names, 2u, &ret, &len, NULL, 0) < 0) ? def : ret;
}
auto UtilObject::totalMemory(MemoryUnit unit) -> double
{
    static const quint64 total = getSysctl(HW_MEMSIZE, (quint64)0);
    return total/(double)unit;
}
int UtilObject::cores() { static const int count = getSysctl(HW_NCPU, 1); return count; }
auto UtilObject::usingMemory(MemoryUnit unit) -> double
{
    task_basic_info info; memset(&info, 0, sizeof(info));
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
        return 0.0;
    return info.resident_size/(double)unit;
}
auto UtilObject::processTime() -> quint64
{
    static const pid_t pid = qApp->applicationPid();
    struct proc_taskinfo info;
    if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info)) < 0)
        return 0;
    return info.pti_total_user/1000 + info.pti_total_system/1000;
}
#endif

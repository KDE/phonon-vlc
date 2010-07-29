#include "vlcmacwidget.h"
#import <Cocoa/Cocoa.h>
#import "nsvideoview.h"

#include <QtDebug>

VlcMacWidget::VlcMacWidget(QWidget *parent) : QMacCocoaViewContainer(0, parent)
{
    // Many Cocoa objects create temporary autorelease objects,
    // so create a pool to catch them.

    printf("[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.[0x123c1b938] vout_macosx generic debug: No drawable-nsobject, passing over.");
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    VideoView *videoView = [[VideoView alloc] init];

    this->setCocoaView(videoView);

    // Release our reference, since our super class takes ownership and we
    // don't need it anymore.
    [videoView release];

    // Clean up our pool as we no longer need it.
    [pool release];

}

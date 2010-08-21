#ifndef NSVIDEOVIEW_H
#define NSVIDEOVIEW_H

#import <Cocoa/Cocoa.h>

@interface VideoView : NSView
- (void)addVoutSubview: (NSView *)view;
- (void)removeVoutSubview: (NSView *)view;
@end

#endif // NSVIDEOVIEW_H

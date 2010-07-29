/**
 * Set the NSView handler where the media player should render its video output.
 *
 * Use the vout called "macosx".
 *
 * The drawable is an NSObject that follow the VLCOpenGLVideoViewEmbedding
 * protocol:
 *
 * @begincode
 * \@protocol VLCOpenGLVideoViewEmbedding <NSObject>
 * - (void)addVoutSubview:(NSView *)view;
 * - (void)removeVoutSubview:(NSView *)view;
 * \@end
 * @endcode
 *
 * Or it can be an NSView object.
 *
 * If you want to use it along with Qt4 see the QMacCocoaViewContainer. Then
 * the following code should work:
 * @begincode
 * {
 *     NSView *video = [[NSView alloc] init];
 *     QMacCocoaViewContainer *container = new QMacCocoaViewContainer(video, parent);
 *     libvlc_media_player_set_nsobject(mp, video);
 *     [video release];
 * }
 * @endcode
 *
 * You can find a live example in VLCVideoView in VLCKit.framework.
 *
 * \param p_mi the Media Player
 * \param drawable the drawable that is either an NSView or an object following
 * the VLCOpenGLVideoViewEmbedding protocol.
 */

#ifndef OPENCMR2_CAMERA_MOVE_H
#define OPENCMR2_CAMERA_MOVE_H
// Debug camera move (visual only). A per-type value video.camera_move_type<N> overrides the family value for camera type N. The key being ABSENT
// means "use the family value"; an explicit 0 is a real override (for example a camera that must not move while the family value is not 0).
// Absence is read with this fallback, which no valid setting (-200..200) equals.
#include <limits.h>
static const int kCamMoveUnset = INT_MIN;
inline int CameraMove_Effective(int perType, int family) { return perType == kCamMoveUnset ? family : perType; }
inline const char *CameraMove_FamilyKey(int cameraType) { return (cameraType >= 4 && cameraType <= 6) ? "video.camera_move_chase" : cameraType == 3 ? "video.camera_move_cockpit" : "video.camera_move_car"; }
#endif

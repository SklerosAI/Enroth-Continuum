#pragma once

namespace mm6 {

/**
 * Runs the GrayFace patch's door action 3 on a door of the current indoor map. A door that is in or going into its
 * alternative state goes back to its initial one, any other door goes into its alternative state. A moving door turns
 * around from where it is.
 *
 * @param doorId                        Id of the door, as in the event script.
 */
void switchDoorEvenIfMoving(unsigned int doorId);

} // namespace mm6

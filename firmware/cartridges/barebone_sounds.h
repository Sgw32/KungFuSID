#include "setup.h"

void error_sound_SD() {
  reset_SID();
}

inline void error_sound_ROOT() {
  reset_SID();
}

inline void error_open_file() {

  reset_SID();
}

inline void error_open_folder () {
  reset_SID();
}

inline void error_PSID_V2_RAM_OVERFLOW () {
  reset_SID();
}

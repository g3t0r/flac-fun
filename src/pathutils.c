#include "pathutils.h"

short isHidden(const char * file) {
  return file[0] == '.';
}

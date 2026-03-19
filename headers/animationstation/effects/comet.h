#ifndef _COMET_H_
#define _COMET_H_

#include "animation.h"
#include "hardware/clocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "animationstation.h"

// Comet animates a lit head along the LED strip (ordered by LED index) with a
// fading tail trailing behind it.  CometBounce reverses direction at each end
// instead of wrapping.
// The tail positions are calculated on-the-fly from CurrentHead + Reversed;
// no position history is needed.

class Comet : public Animation {
public:
  Comet(Lights& InRGBLights, EButtonCaseEffectType InButtonCaseEffectType, bool InBounce);
  ~Comet() {};

  virtual void Animate(RGB (&frame)[FRAME_MAX]) override;

  virtual void CycleParameterChange() override;

  bool BounceMode = false;

protected:

  void CheckForEnd();

  std::vector<uint32_t> OrderedLights; // sorted by FirstLedIndex ascending

  int CurrentHead = 0;
  bool Reversed = false;

  float HeadTime = 1.0f;

  int RainbowWheelFrame = 0;
  bool RainbowWheelReversed = false;
};

#endif

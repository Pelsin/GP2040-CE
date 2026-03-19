#include "comet.h"
#include <algorithm>
#include <cstdlib>

#define COMET_CYCLE_MAX 900
#define COMET_CYCLE_MIN 10

#define COMET_RAINBOW_FRAME_CHANGE_PER_TICK 10

Comet::Comet(Lights &InRGBLights, EButtonCaseEffectType InButtonCaseEffectType, bool InBounce)
		: Animation(InRGBLights, InButtonCaseEffectType)
{
	BounceMode = InBounce;

	CycleParameterChange();

	// Build list of lights valid for this animation, sorted by LED index so the
	// comet travels in physical strip order regardless of grid coordinates.
	for (unsigned int lightIndex = 0; lightIndex < RGBLights->AllLights.size(); ++lightIndex)
	{
		if (LightTypeIsForAnimation(RGBLights->AllLights[lightIndex].Type) == false)
			continue;
		OrderedLights.push_back(lightIndex);
	}

	std::sort(OrderedLights.begin(), OrderedLights.end(), [this](uint32_t a, uint32_t b)
						{ return RGBLights->AllLights[a].FirstLedIndex < RGBLights->AllLights[b].FirstLedIndex; });

	CurrentHead = 0;
	HeadTime = 1.0f;
}

void Comet::Animate(RGB (&frame)[FRAME_MAX])
{
	UpdateTime();

	if (OrderedLights.size() < 2)
		return;

	for (unsigned int i = 0; i < OrderedLights.size(); ++i)
	{
		uint32_t lightIndex = OrderedLights[i];
		uint8_t firstLed = RGBLights->AllLights[lightIndex].FirstLedIndex;
		uint8_t lastLed = firstLed + RGBLights->AllLights[lightIndex].LedsPerLight;
		for (uint8_t led = firstLed; led < lastLed; ++led)
			frame[led] = GetNonPressedColorForLight(lightIndex);
	}

	RGB cometCol = AnimationStation::options.profiles[AnimationStation::options.baseProfileIndex].nonPressedSpecialColor;
	if (AnimationStation::options.profiles[AnimationStation::options.baseProfileIndex].bNonPressedSpecialColorIsRainbow)
	{
		if (!RainbowWheelReversed)
		{
			RainbowWheelFrame += COMET_RAINBOW_FRAME_CHANGE_PER_TICK;
			if (RainbowWheelFrame >= 255)
			{
				RainbowWheelReversed = true;
				RainbowWheelFrame = 255;
			}
		}
		else
		{
			RainbowWheelFrame -= COMET_RAINBOW_FRAME_CHANGE_PER_TICK;
			if (RainbowWheelFrame <= 0)
			{
				RainbowWheelReversed = false;
				RainbowWheelFrame = 0;
			}
		}
		cometCol = RGB::wheel(RainbowWheelFrame);
	}

	int tailLengthPercent = AnimationStation::options.profiles[AnimationStation::options.baseProfileIndex].tailLength;
	int tailLen = (tailLengthPercent * (int)OrderedLights.size()) / 100;
	const int lightCount = (int)OrderedLights.size();

	auto tailPos = [&](int t) -> int
	{
		if (!BounceMode)
		{
			return ((CurrentHead - t) % lightCount + lightCount) % lightCount;
		}
		else
		{
			const int period = 2 * (lightCount - 1);
			int linear = Reversed ? (period - CurrentHead) : CurrentHead;
			linear = ((linear - t) % period + period) % period;
			return linear < lightCount ? linear : period - linear;
		}
	};

	for (int t = 1; t <= tailLen; ++t)
	{
		float alpha = 1.0f - (float)t / (float)(tailLen + 1);
		int tailActual = tailPos(t);
		uint32_t lightIndex = OrderedLights[tailActual];
		RGB baseColor = GetNonPressedColorForLight(lightIndex);
		uint8_t firstLed = RGBLights->AllLights[lightIndex].FirstLedIndex;
		uint8_t lastLed = firstLed + RGBLights->AllLights[lightIndex].LedsPerLight;
		for (uint8_t led = firstLed; led < lastLed; ++led)
			frame[led] = BlendColor(baseColor, cometCol, alpha);
	}

	// Draw head last so it is never overwritten by the tail.
	{
		uint32_t lightIndex = OrderedLights[CurrentHead];
		uint8_t firstLed = RGBLights->AllLights[lightIndex].FirstLedIndex;
		uint8_t lastLed = firstLed + RGBLights->AllLights[lightIndex].LedsPerLight;
		for (uint8_t led = firstLed; led < lastLed; ++led)
			frame[led] = cometCol;
	}

	HeadTime -= ((float)cycleTime / 1000.0f);
	if (HeadTime <= 0.0f)
	{
		HeadTime = 1.0f;
		if (!Reversed)
			CurrentHead++;
		else
			CurrentHead--;
		CheckForEnd();
	}
}

void Comet::CheckForEnd()
{
	const int last = (int)OrderedLights.size() - 1;
	if (!Reversed && CurrentHead > last)
	{
		CurrentHead = BounceMode ? last : 0;
		Reversed = BounceMode;
	}
	else if (Reversed && CurrentHead < 0)
	{
		Reversed = false;
		CurrentHead = 0;
	}
}

void Comet::CycleParameterChange()
{
	int16_t cycleStep;
	if (ButtonCaseEffectType == EButtonCaseEffectType::BUTTONCASELIGHTTYPE_CASE_ONLY)
		cycleStep = AnimationStation::options.profiles[AnimationStation::options.baseProfileIndex].baseCaseCycleTime;
	else
		cycleStep = AnimationStation::options.profiles[AnimationStation::options.baseProfileIndex].baseCycleTime;

	cycleTime = COMET_CYCLE_MIN + (((COMET_CYCLE_MAX - COMET_CYCLE_MIN) / CYCLE_STEPS) * cycleStep);
}

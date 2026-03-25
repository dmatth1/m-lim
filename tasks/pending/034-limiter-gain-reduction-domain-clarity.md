# Task 034: Limiter Gain Reduction — Domain and Knee Math Clarity

## Description
Tasks 011 and 012 mention "Gain is applied in linear domain (multiply, not add in dB)" which is correct, but don't clearly specify that the envelope follower / gain computer should work in the dB (log) domain. This is a critical distinction:

1. **Gain computation** (envelope follower, attack/release smoothing): must operate in dB domain to get correct exponential behavior
2. **Gain application** (to audio samples): must convert dB gain to linear and multiply

If a worker implements the envelope follower in linear domain, the attack/release curves will be wrong (linear decay instead of exponential dB decay), producing audible pumping artifacts.

Also, the soft knee math needs explicit formulas. Task 011 says "smooth transition from no-reduction to full-reduction based on kneeWidth" but doesn't give the knee equation.

## Produces
None

## Consumes
TransientLimiterInterface
LevelingLimiterInterface

## Relevant Files
Modify: `tasks/pending/011-transient-limiter.md` — Add explicit gain computation math
Modify: `tasks/pending/012-leveling-limiter.md` — Add explicit envelope follower math
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — Verify gain computation domain (if implemented)
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — Verify envelope follower domain (if implemented)

## Acceptance Criteria
- [ ] Run: `grep -c "Decibels\|dB\|log\|20.0f \* std::log10\|juce::Decibels" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: at least 2 (gain computed in dB domain)
- [ ] Run: `grep -c "Decibels\|gainToDecibels\|decibelsToGain" M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: at least 2

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_release_curve_is_exponential_db` — After a peak, gain reduction in dB should decrease linearly over time (exponential in linear domain)

## Technical Details
### Gain Computer (TransientLimiter)
For input level `x_dB` and threshold `T_dB` (0 dBFS for ceiling):

**Hard knee:**
```
if x_dB <= T_dB: gain_dB = 0
else: gain_dB = T_dB - x_dB
```

**Soft knee** (kneeWidth `W` in dB):
```
if x_dB < T_dB - W/2: gain_dB = 0
elif x_dB > T_dB + W/2: gain_dB = T_dB - x_dB
else: gain_dB = (1/(2*W)) * (x_dB - T_dB + W/2)^2  // note: this is negative
```

### Envelope Follower (both stages)
The attack/release smoothing operates on `gain_dB`:
```
if gain_dB < envelope_dB:  // attack (more reduction)
    envelope_dB = attackCoeff * envelope_dB + (1 - attackCoeff) * gain_dB
else:  // release (less reduction)
    envelope_dB = releaseCoeff * envelope_dB + (1 - releaseCoeff) * gain_dB
```

### Gain Application
```
float linearGain = juce::Decibels::decibelsToGain(envelope_dB);
sample *= linearGain;
```

Workers must NOT smooth the gain in linear domain — this produces incorrect attack/release curves.

## Dependencies
None (plan-level clarification, should be done before tasks 011, 012 are started)

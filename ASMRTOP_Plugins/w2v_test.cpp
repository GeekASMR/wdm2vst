#include <JuceHeader.h>
int main() {
    juce::LagrangeInterpolator r;
    const float in[100] = {0};
    float out[100] = {0};
    int consumed = r.process(1.5, in, out, 50);
    return consumed;
}

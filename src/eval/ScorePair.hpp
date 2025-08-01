// ScorePair.hpp: Compact way of storing tapered evaluation terms

#ifndef SCOREPAIR_HPP
#define SCOREPAIR_HPP

class ScorePair
{
    public:
        // Constructor
        ScorePair() = default;
        ScorePair(int mg, int eg) {
            mg_value = mg;
            eg_value = eg;
        }

        constexpr int mg() const { return mg_value; }
        constexpr int eg() const { return eg_value; }
        int interpolate(uint8_t phase) { return (mg_value * phase + eg_value * (64 - phase)) / 64; }

    private:
        int16_t mg_value; 
        int16_t eg_value;
};

#define S(mg, eg) (ScorePair((mg), (eg)))

#endif // SCOREPAIR_HPP